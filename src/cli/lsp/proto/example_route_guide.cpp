/**
 * aaltitoad - a verification engine for tick tock automata models
   Copyright (C) 2023 Asger Gitz-Johansen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "example_route_guide.h"
#include <sstream>

namespace aaltitoad::lsp::proto {
    example_route_guide_t::example_route_guide_t() {
        std::cout << "ctor example_route_guide" << std::endl;
    }

    grpc::Status example_route_guide_t::GetFeature(grpc::ServerContext* server, const FeatureLookup* lookup, Feature* result) {
        auto ss = std::stringstream{};
        ss << "from aaltitoad " << lookup->identifier();
        result->set_name(ss.str());
        std::cout << "got request " << lookup->identifier() << std::endl;
        return grpc::Status::OK;
    }

    void example_route_guide_t::start(int port) {
        auto ss = std::stringstream{};
        ss << "0.0.0.0:" << port;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(ss.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(this);
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << ss.str() << std::endl;
        server->Wait();
    }
}
