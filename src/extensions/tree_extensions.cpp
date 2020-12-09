/**
 * Copyright (C) 2020 Asger Gitz-Johansen

   This file is part of aaltitoad.

    aaltitoad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aaltitoad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aaltitoad.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "tree_extensions.h"

void ConverterHelper(std::stringstream& ss, const Tree<ASTNode>& n) {
    switch (n.root.type) {
        case NodeType_t::Var:
        case NodeType_t::Location:
        case NodeType_t::Deadlock:
        case NodeType_t::Literal:
            ss << n.root.token; break;
        case NodeType_t::LogicOr:
        case NodeType_t::CompLess:
        case NodeType_t::CompLessEq:
        case NodeType_t::CompNeq:
        case NodeType_t::CompEq:
        case NodeType_t::CompGreater:
        case NodeType_t::CompGreaterEq:
        case NodeType_t::Operator:
        case NodeType_t::LogicAnd:
            ConverterHelper(ss, n.children[0]);
            ss << " " << n.root.token << " ";
            ConverterHelper(ss, n.children[1]);break;
        case NodeType_t::SubExpr:
            ss << "("; ConverterHelper(ss, n.children[0]); ss << ")";break;
        case NodeType_t::Negation:
        case NodeType_t::Finally:
        case NodeType_t::Globally:
        case NodeType_t::Next:
        case NodeType_t::Until:
        case NodeType_t::Exists:
        case NodeType_t::Forall:
            ss << n.root.token << " ";
            ConverterHelper(ss, n.children[0]);break;
        case NodeType_t::UNKNOWN:
        default: ss << n.root.token << " ";break;
    }
}

std::string ConvertASTToString(const Tree<ASTNode>& tree) {
    std::stringstream ss{};
    ConverterHelper(ss, tree);
    return ss.str();
}
