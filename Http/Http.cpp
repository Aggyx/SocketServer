#include "../Http/Http.hpp"
#include "../Logger/Logger.hpp"

// variables locales al archivo
namespace {
    Http* g_http_instance = NULL;  

    void signal_handler(int signum) {
        if (g_http_instance) {
            g_http_instance->free_servers();
            exit(signum);
        }
    }
}


// FORMA DE COPLIEN
Http::Http()
{

}

Http::Http(const Http& copy)
{
    (void)copy;
}
Http&   Http::operator=(const Http& assign)
{
    (void)assign;
    return (*this);
}

Http::~Http()
{
    quitSignal();
}

// CLASE DE ERROR de Htpp
Http::ServerError::ServerError(const std::string& error_descr)
{
    error_string = error_descr;
}

Http::ServerError::~ServerError() throw()
{
    
}

const char * Http::ServerError::what() const throw()
{
	return error_string.c_str();
}

// METHODOS
void    Http::configure(const std::string&  configfile)
{
    std::vector<ServerConfig> serverConfigs = ConfigParser::parseServerConfigFile(configfile);

    if (serverConfigs.empty())
    {
        LOG("Configuration file does not have a valid Server Configuration.");
        return ;
    }
    
    try{
        
        for (std::vector<ServerConfig>::const_iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
            Server* server = new Server(*it);
            servers.push_back(server);
        }

    } catch (const std::exception& e) {
        LOG(e.what());
    }
    if (servers.empty())
        LOG("No servers could be started.");

}

void    Http::free_servers()
{
    for (std::list<Server*>::iterator it = servers.begin(); it != servers.end(); it++)
        delete (*it);
    LOG_INFO("Shutting down servers ...");
    servers.clear();
}

void    Http::quitSignal()
{
    free_servers();
}

void Http::setupSignalHandlers(Http* http)
{
    g_http_instance = http;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void Http::launch_all()
{
    // Verificar si hay servidores configurados
    if (servers.empty())
        return;

    try {
        for (std::list<Server*>::iterator it = servers.begin(); it != servers.end(); it++) {
            (*it)->init();
        }
    } catch (std::exception& e) {
        LOG(e.what());
        return;
    }
    LOG_INFO("All servers are launching... Press CTRL+C to quit");

    std::vector<pollfd> master_fds;

    while (true) {
        // Reconstruir master_fds para reflejar correctamente los sockets pasivos y activos
        master_fds.clear();
        for (std::list<Server*>::iterator srv_it = servers.begin(); srv_it != servers.end(); srv_it++) {
            // Agregar sockets pasivos del servidor
            std::vector<int> server_fds = (*srv_it)->getPassiveSocketFd();
            for (std::vector<int>::iterator fd_it = server_fds.begin(); fd_it != server_fds.end(); fd_it++) {
                pollfd server_fd = {*fd_it, POLLIN, 0};
                master_fds.push_back(server_fd);
            }

            // Agregar sockets activos de clientes
            for (std::vector<ClientInfo*>::iterator cli_it = (*srv_it)->clients.begin(); 
                 cli_it != (*srv_it)->clients.end(); cli_it++) {
                pollfd client_fd = {(*cli_it)->pfd.fd, POLLIN, 0};
                master_fds.push_back(client_fd);
            }
        }

        // Llamar a poll() para verificar eventos
        int poll_count = poll(&master_fds[0], master_fds.size(), 1000); // Timeout de 1000ms
        if (poll_count < 0) {
            if (errno != EINTR) {
                LOG("Poll error in master loop !!!");
            }
            continue;
        }

        size_t fd_index = 0;

        // Manejar eventos en los sockets pasivos de los servidores
        for (std::list<Server*>::iterator srv_it = servers.begin(); srv_it != servers.end(); srv_it++) {
            std::vector<int> server_fds = (*srv_it)->getPassiveSocketFd();
            for (std::vector<int>::iterator fd_it = server_fds.begin(); fd_it != server_fds.end(); fd_it++, fd_index++) {
                if (master_fds[fd_index].revents & POLLIN) {
                    (*srv_it)->acceptClient(*fd_it);
                }
            }
        }

        // Manejar eventos en los sockets activos de los clientes
        for (std::list<Server*>::iterator srv_it = servers.begin(); srv_it != servers.end(); srv_it++) {
            std::vector<ClientInfo*>::iterator cli_it = (*srv_it)->clients.begin();
            while (cli_it != (*srv_it)->clients.end()) {
                if (fd_index >= master_fds.size()) {
                    LOG_INFO("Client has not been polled: Index out of bounds");
                    break;
                }

                if ((*srv_it)->IsTimeout(*cli_it)) {
                    // Enviar respuesta de timeout antes de cerrar la conexión
                    std::string timeout_response = "HTTP/1.1 408 Request Timeout\r\n"
                                                   "Content-Length: 0\r\n\r\n";
                    send((*cli_it)->pfd.fd, timeout_response.c_str(), timeout_response.size(), 0);
                    close((*cli_it)->pfd.fd);
                    delete *cli_it;
                    cli_it = (*srv_it)->clients.erase(cli_it);
                    LOG_INFO("Client Timeouted");
                    continue;
                }

                if (master_fds[fd_index].revents & POLLIN) {
                    try {
                        (*srv_it)->handleClient(*cli_it);
                    } catch (const std::exception &e) {
                        LOG_INFO(std::string("Error handling client: ") + e.what());
                        close((*cli_it)->pfd.fd);
                        delete *cli_it;
                        cli_it = (*srv_it)->clients.erase(cli_it);
                        continue;
                    }
                } else if (master_fds[fd_index].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    close((*cli_it)->pfd.fd);
                    delete *cli_it;
                    cli_it = (*srv_it)->clients.erase(cli_it);
                    LOG_INFO("Connection Removed");
                    continue;
                }

                cli_it++;
                fd_index++;
            }
        }
    }
}
