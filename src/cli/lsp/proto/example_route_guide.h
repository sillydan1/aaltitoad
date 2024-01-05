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
#ifndef EXAMPLE_ROUTE_GUIDE_H
#define EXAMPLE_ROUTE_GUIDE_H
#include <aaltitoad.pb.h>
#include <aaltitoad.grpc.pb.h>
#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/support/status.h>

namespace aaltitoad::lsp::proto {
    class example_route_guide_t final : public RouteGuide::Service {
    public:
         example_route_guide_t();
         grpc::Status GetFeature(grpc::ServerContext* server, const FeatureLookup* lookup, Feature* result) override;
         // NOTE: blocking call
         void start(int port);
    };
}

#endif // !EXAMPLE_ROUTE_GUIDE_H
