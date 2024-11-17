/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   GetHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smagniny <santi.mag777@student.42madrid    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/12 18:44:29 by smagniny          #+#    #+#             */
/*   Updated: 2024/11/17 01:11:05 by smagniny         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Handlers.hpp"
#include "../Router/Router.hpp"

std::string     readFile(const std::string& path)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        std::ifstream error_file("/var/www/error-pages/404.html");
        if (!error_file.is_open())
            return "404 not a page";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string formatDateTime(time_t time) {
    struct tm* timeInfo = localtime(&time);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
    return std::string(buffer);
}

std::string formatSize(off_t size) {
    std::stringstream ss;
    ss << size << " B";
    return ss.str();
}

std::vector<std::string> listFiles(const std::string& directoryPath) {
    std::vector<std::string> files;

    // Abrir el directorio
    std::cout << " directorio" << directoryPath <<std::endl;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir == NULL) {
        std::cerr << "Error: No se pudo abrir el directorio " << directoryPath << std::endl;
        return files;  // Retorna un vector vacío si no se puede abrir el directorio
    }

    // Leer los archivos en el directorio
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Excluir "." y ".." que representan el directorio actual y el directorio padre
        if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
            files.push_back(entry->d_name);  // Agregar el nombre del archivo al vector
        }
    }

    // Cerrar el directorio
    closedir(dir);
    return files;
}

std::string generateAutoIndex(const std::string& directory) {
    //std::cout << "Generando autoindex para el directorio: " << directory << std::endl;

    std::string autoindexHtml = "<html><head><style>";
    autoindexHtml += "table { width: 100%; border-collapse: collapse; }";
    autoindexHtml += "th, td { padding: 8px 12px; border-bottom: 1px solid #ddd; text-align: left; }";
    autoindexHtml += "th { background-color: #f2f2f2; font-weight: bold; }";
    autoindexHtml += "tr:hover { background-color: #f9f9f9; }";
    autoindexHtml += "</style></head><body>";
    autoindexHtml += "<h1>Index of " + directory + "</h1><table>";
    autoindexHtml += "<tr><th style='width: 50%;'>Name</th><th style='width: 20%;'>Size</th><th style='width: 30%;'>Last Modified</th></tr>";
    DIR* dir = opendir(directory.c_str());
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                std::string filepath = directory + "/" + filename;

                struct stat fileStat;
                if (stat(filepath.c_str(), &fileStat) == 0) {
                    autoindexHtml += "<tr><td><a href=\"" + filename + "\">" + filename + "</a></td>"; // Ruta relativa
                    autoindexHtml += "<td>" + formatSize(fileStat.st_size) + "</td>"; // Tamaño del archivo
                    autoindexHtml += "<td>" + formatDateTime(fileStat.st_mtime) + "</td>"; // Fecha de modificación
                    autoindexHtml += "</tr>";
                } else {
                    std::cerr << "No se pudo obtener información del archivo: " << filename << std::endl;
                }
            }
        }
        closedir(dir);
    } else {
        // Error al abrir el directorio
        autoindexHtml = "<html><body><h1>Error: Cannot open directory</h1></body></html>";
        std::cerr << "Error al abrir el directorio: " << strerror(errno) << std::endl;
    }

    autoindexHtml += "</table></body></html>";
    return autoindexHtml;
}

std::string generateAutoIndexDelete(const std::string& directoryPath, const LocationConfig& locationConfig) {
    std::string autoindexHtml = "<html><body><h1>Directory Listing</h1><table>";

    std::vector<std::string> files = listFiles(directoryPath);

    for (std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it)
    {
        const std::string& filename = *it;
        std::string filepath = directoryPath + filename;  
        std::cout << "filepath en autoindex delete" << filepath << std::endl;
        autoindexHtml += "<tr>";
        autoindexHtml += "<td>" + filename + "</td>";

        // Crear un formulario para la eliminación con método POST
        autoindexHtml += "<td>";
        autoindexHtml += "<form action='/delete' method='POST'>";
        autoindexHtml += "<input type='hidden' name='archivo' value=";
        autoindexHtml += "'" + filepath + "'>";
        autoindexHtml += "<input type='submit' name='boton' value='Delete'>";// Pasamos la ruta del archivo
        autoindexHtml += "</form>";
        autoindexHtml += "</td>";

        autoindexHtml += "</tr>";
    }

    autoindexHtml += "</table></body></html>";

    return autoindexHtml;
}

GetHandler::GetHandler()
{    
}

GetHandler::~GetHandler()
{
    
    
}


void GetHandler::handle(const Request* request, Response* response, const LocationConfig& locationconfig) {
    //std::cout << "Received GET request" << std::endl;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string fullpath(cwd);
        std::string requested_resource = request->getUri();
        //std::cout << requested_resource << std::endl;
        
        fullpath += locationconfig.root;
    
        //std::cout << " cgi_pass:: " << locationconfig.cgi_pass << std::endl;
        LOG_INFO(fullpath);
        if (locationconfig.cgi_pass.empty() == false)
        {
            CgiHandler cgi_handler_instance;
            cgi_handler_instance.handle(request, response, locationconfig);
            response->setStatusCode(201);
            LOG_INFO("CGI Resource");
            return ;
        }
        if (!locationconfig.redirect.empty())
        {
            response->setHeader("Location", locationconfig.redirect);
            std::ostringstream oss;
            oss << locationconfig.redirect_type;
            std::string body = "<html><body><h1>" + oss.str() + " " +
                            (locationconfig.redirect_type == 301 ? "Moved Permanently" : "Found") +
                            "</h1></body></html>";
            response->setBody(body);
            response->setStatusCode(locationconfig.redirect_type);
            return;
        }
        else if (fullpath[fullpath.size() - 1] == '/')
        {
            struct stat buffer;

            if (request->getUri().compare("/delete") == 0)
            {
                if (locationconfig.autoindex) {
                    std::string fileContent = generateAutoIndexDelete(fullpath, locationconfig);
                    response->setStatusCode(200);
                    response->setBody(fileContent);
                    response->setHeader("Content-Type", "text/html");
                    LOG_INFO("AUTOINDEX UPLOAD resource");
                } else {
                    LOG_INFO("Forbidden resource");
                    response->setStatusCode(403);
                    response->setBody(readFile("/var/www/error-pages/403.html"));
                }
                return ;

            }
            if (stat(fullpath.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode)) {
                if (locationconfig.autoindex) {
                    std::string fileContent = generateAutoIndex(fullpath);
                    response->setStatusCode(200);
                    response->setBody(fileContent);
                    response->setHeader("Content-Type", "text/html");
                    LOG_INFO("AUTOINDEX resource");
                } else {
                    LOG_INFO("Forbidden resource");
                    response->setStatusCode(403);
                    response->setBody(readFile("/var/www/error-pages/403.html"));
                }
                return ;
            }
        }

        else{
            
            if (access(fullpath.c_str(), F_OK) != 0) {
                response->setStatusCode(404);
                response->setBody(readFile("/var/www/error-pages/404.html"));
            }
            //std::cout << fullpath.c_str() << std::endl;
            std::ifstream file(fullpath.c_str());
            if (!file.is_open())
            {
                response->setStatusCode(404);
                response->setBody(readFile("/var/www/error-pages/404.html"));
            }
            
            std::stringstream bufferStream;
            bufferStream << file.rdbuf();
            
            if (!(bufferStream.str().empty()))
            {
                response->setStatusCode(200);
                response->setBody(bufferStream.str());
                response->setHeader("Content-Type", response->getMimeType(fullpath));
                LOG_INFO("Read Resource Succesfully");
            }
        }
    }
    else {
        LOG_INFO("Failed current Directory Read");
        response->setStatusCode(500);
        response->setBody(readFile("/var/www/error-pages/500.html"));
    }
}
