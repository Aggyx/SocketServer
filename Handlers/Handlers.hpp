/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Handlers.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smagniny <smagniny@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/20 20:09:58 by smagniny          #+#    #+#             */
/*   Updated: 2024/11/27 17:40:17 by smagniny         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HANDLERS_HPP
#define HANDLERS_HPP

// Forward declarations
class Request;
class Response;
class LocationConfig;

#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <cstdio>
#include <cstring>

#include "../Request/Request.hpp"
#include "../Response/Response.hpp"
#include "../Config/ConfigParser.hpp"
#include "../Handlers/cgiHandler.hpp"

class RequestHandler {
public:
    virtual ~RequestHandler() {}
    virtual void handle(const Request* request, Response* response, LocationConfig& locationconfig) = 0;
};

class GetHandler : public RequestHandler
{
    public:
        GetHandler();
        ~GetHandler();
        
        virtual void        handle(const Request* request, Response* response, LocationConfig& locationconfig);        
};

class PostHandler : public RequestHandler 
{
    public:
        PostHandler();
        ~PostHandler();
        virtual void        handle(const Request* request, Response* response, LocationConfig& locationconfig);
    private:
            bool                                saveFile(const std::string& filename, const std::string& data);
            std::map<std::string, std::string>  parseUrlFormData(const std::string& body);
            std::string                         parseMultipartFormData(const std::string& data, const std::string& boundary, const std::string& post_upload_store, std::string& filename);
            void                                appendUsertoDatabase(std::map<std::string, std::string>& formData, Response& response, const LocationConfig& locationconfig);
            void                                handleMultipartFormData(const Request* request, Response* response, LocationConfig& locationconfig);
            void                                handleUrlFormEncoded(const Request* request, Response* response, LocationConfig& locationconfig);
};

class DeleteHandler : public RequestHandler {
public:
    DeleteHandler();
    virtual ~DeleteHandler();
    virtual void    handle(const Request* request, Response* response, LocationConfig& locationconfig);
    static  void    remove_file_or_dir(Response* response, const LocationConfig& locationconfig);

private:
    // bool validatePath(const std::string& path, const std::string& root) const;
    // bool deleteResource(const std::string& path, Response* response);
    // std::string constructResponseHtml(const std::string& filename, bool success);
}; 

std::string                         urlDecode(const std::string &str);
std::string                         escapeHtml(const std::string& data);
std::string                         readFile(const std::string& path);

#endif
