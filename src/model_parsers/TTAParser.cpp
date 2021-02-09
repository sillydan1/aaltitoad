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
#include <aaltitoadpch.h>
#include <extensions/stringextensions.h>
#include <extensions/overload>
#include <json_parsing/JSONParser.h>

#include <model_parsers/TTAParser.h>

// Some json files are important to ignore, so Ignore-list:
std::vector<std::string> componentIgnoreList = { // NOLINT(cert-err58-cpp)
        "ignore",       // ignore all files that want to be ignored
        "Queries.json", // Queries are not component- or symbol-files
        ".parts",       // parts files are not components
};

bool ShouldSkipEntry(const std::filesystem::__cxx11::directory_entry& entry) {
    auto entrystr = entry.path().string();
    return  !entry.is_regular_file() ||
            std::any_of(componentIgnoreList.begin(), componentIgnoreList.end(),
                        [&entrystr] (auto& el) { return entrystr.find(el) != std::string::npos; });
}

TTAIR_t TTAParser::ParseToIntermediateRep(const std::string &path) {
    TTAIR_t ttair{};
    AddInternalAndExternalSymbols(path, ttair);
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        if(ShouldSkipEntry(entry)) continue;
        auto parsedComponent = ParseComponent(entry.path().generic_string());
        if(parsedComponent)
            ttair.AddComponent(std::move(parsedComponent.value()));
    }
    return ttair;
}

void TTAParser::AddInternalAndExternalSymbols(const std::string& path, TTAIR_t& ttair) {
    auto internalParts = ParsePartsFiles(path);
    auto extractedParts = ExtractExternalParts(internalParts);
    ttair.AddInternalSymbols(std::move(extractedParts.internal));
    ttair.AddExternalSymbols(std::move(extractedParts.external));
}

std::vector<TTAParser::SymbolExternalPair> TTAParser::ParsePartsFiles(const std::string &path) {
    std::vector<TTAParser::SymbolExternalPair> totalParts{};
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        // Filter over ONLY the .parts files.
        auto isEntryPartsFile = entry.is_regular_file() && entry.path().string().find(".parts") != std::string::npos;
        if(!isEntryPartsFile) continue;
        auto parts = ParsePartsFile(entry.path().string());
        totalParts.insert(totalParts.end(), parts.begin(), parts.end());
    }
    return totalParts;
}

std::vector<TTAParser::SymbolExternalPair> TTAParser::ParsePartsFile(const std::string &filepath) {
    std::ifstream file{filepath};
    if(file) {
        auto dom_doc = JSONParser::ParseDocumentDOMStyle(file);
        std::vector<TTAParser::SymbolExternalPair> symbols{};
        if(IsDocumentAProperPartsFile(dom_doc)) {
            auto partsArray = dom_doc["parts"].GetArray();
            std::for_each(partsArray.begin(), partsArray.end(),
                          [&symbols](const auto& doc){ symbols.emplace_back(std::move(ParsePart(doc))); });
        } else
            spdlog::error("Parts File '{0}' is improper", filepath);
        file.close();
        return symbols;
    }
    return {};
}

TTAParser::ExtractedSymbolLists TTAParser::ExtractExternalParts(std::vector<TTAParser::SymbolExternalPair>& allParts) {
    TTAParser::ExtractedSymbolLists lsts{};
    for(auto& it : allParts) {
        if(it.isExternal)   lsts.external.emplace_back(std::move(it.symbol));
        else                lsts.internal.emplace_back(std::move(it.symbol));
    }
    return lsts;
}

TTAIR_t::Location ParseLocation(const rapidjson::Document::ValueType& elem, const std::string& elemname) {
    auto element = elem.FindMember(elemname.c_str());
    return { .isImmediate = element->value["urgency"].GetString() == std::string("urgent"),
             .identifier = element->value["id"].GetString() };
}

std::optional<TTAIR_t::Component> TTAParser::ParseComponent(const std::string &filepath) {
    std::ifstream file{filepath};
    if(file) {
        spdlog::debug("Parsing file '{0}' as a JSON file", filepath);
        auto dom_document = JSONParser::ParseDocumentDOMStyle(file);
        // Ensure existence of required high-level members
        if(IsDocumentAProperTTA(dom_document)) {
            spdlog::debug("File '{0}' is a proper TTA", filepath);
            return std::optional(TTAIR_t::Component{
                    .name = GetFileNameOnly(filepath),
                    .initialLocation = ParseLocation(dom_document, "initial_location"),
                    .endLocation     = ParseLocation(dom_document, "final_location"),
                    .isMain = dom_document["main"].GetBool(),
                    .edges = ParseEdges(dom_document["edges"], dom_document),
                    .symbols = ParseSymbolDeclarations(dom_document)
            });
        } else
            spdlog::error("File '{0}' is improper", filepath);
        file.close();
        return {};
    }
    spdlog::error("File '{0}' does not exist", filepath);
    return {};
}

bool TTAParser::IsDocumentAProperTTA(const rapidjson::Document &document) {
    return  document.IsObject() &&
            JSONParser::DoesMemberExistAndIsObject(document, "initial_location") &&
            JSONParser::DoesMemberExistAndIsString(document["initial_location"], "id") &&
            JSONParser::DoesMemberExistAndIsObject(document, "final_location") &&
            JSONParser::DoesMemberExistAndIsString(document["final_location"], "id") &&
            JSONParser::DoesMemberExistAndIsArray( document, "edges") &&
            JSONParser::DoesMemberExistAndIsBool(  document, "main") &&
            JSONParser::DoesMemberExistAndIsString(document, "declarations") &&
            JSONParser::DoesMemberExistAndIsArray(document, "locations") &&
            IsProperLocationList(document["locations"].GetArray());
}

bool TTAParser::IsProperLocationList(const rapidjson::Document::ConstArray &locationList) {
    bool accumulator = true;
    for(auto locationObject = locationList.begin(); locationObject != locationList.end() || !accumulator; locationObject++) {
        accumulator = accumulator && JSONParser::DoesMemberExistAndIsString(*locationObject, "id");
        accumulator = accumulator && JSONParser::DoesMemberExistAndIsString(*locationObject, "urgency");
    }
    return accumulator;
}

std::vector<TTAIR_t::Edge>
TTAParser::ParseEdges(const rapidjson::Document::ValueType &edgeList, const rapidjson::Document& document) {
    std::vector<TTAIR_t::Edge> edges{};
    for(auto edge = edgeList.Begin(); edge != edgeList.End(); edge++)
        edges.push_back(ParseEdge(*edge, document));
    return edges;
}

std::optional<const rapidjson::Document::ValueType*>
TTAParser::FindLocationWithName(const rapidjson::Document& document, const std::string& query_name) {
    if(document["initial_location"]["id"].GetString() == query_name)
        return { &document["initial_location"] };
    else if(document["final_location"]["id"].GetString() == query_name)
        return { &document["final_location"] };

    auto array = document["locations"].GetArray();
    for(int i = 0; i < array.Size(); i++) {
        auto id = array[i].FindMember("id");
        if(id->value.GetString() == query_name) return { &array[i] };
    }
    spdlog::error("Unable to find Location with 'id':'{0}' - Make sure that the location names are correct!", query_name);
    return {};
}

TTAIR_t::Edge TTAParser::ParseEdge(const rapidjson::Document::ValueType &edge, const rapidjson::Document& document) {
    auto sourceLocation = FindLocationWithName(document, edge["source_location"].GetString());
    auto targetLocation = FindLocationWithName(document, edge["target_location"].GetString());
    if(!sourceLocation) throw std::logic_error("Unable to locate source location for edge");
    if(!targetLocation) throw std::logic_error("Unable to locate target location for edge");
    return TTAIR_t::Edge{
            .sourceLocation = {
                    .isImmediate = (*sourceLocation.value())["urgency"].GetString() == std::string("urgent"),
                    .identifier = (*sourceLocation.value())["id"].GetString()
            },
             .targetLocation = {
                    .isImmediate = (*targetLocation.value())["urgency"].GetString() == std::string("urgent"),
                    .identifier = (*targetLocation.value())["id"].GetString()
            },
            .guardExpression = edge["guard"].GetString(),
            .updateExpression = edge["update"].GetString()};
}

std::vector<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclarations(const rapidjson::Document &document) {
    std::vector<TTAIR_t::Symbol> symbols{};
    auto decls = document["declarations"].GetString();
    if(decls != std::string("")) {
        auto declarationLines = split(decls, '\n');
        for(auto& decl : declarationLines) {
            auto symbol = ParseSymbolDeclaration(decl);
            if(symbol) symbols.emplace_back(std::move(symbol.value()));
            // Improper symbol decls should result in fatal error
            else       throw std::logic_error("Symbol declaration failure");
        }
    }
    return symbols;
}

std::optional<TTAIR_t::Symbol> TTAParser::ParseSymbolDeclaration(const std::string &declaration) {
    auto tokens = split(declaration, ' ');
    if(tokens.size() < 5) {
        spdlog::critical("Variable declaration '{0}' is missing tokens - The format is "
                      "[<Access modifier><Space><Type><Space><Identifier><Space>:=<Space><Initial Value>]",
                      declaration);
        return {};
    }
    if(tokens[3] != ":=") {
        spdlog::critical("Variable declaration '{0}' does not contain a ':=' token", declaration);
        return {};
    }
    auto value = TTASymbolValueFromTypeAndValueStrings(tokens[1], tokens[4]);
    auto identifier = tokens[2];
    return TTAIR_t::Symbol{.identifier=identifier,.value=value};
}

TTA TTAParser::ConvertToModelType(const TTAIR_t &intermediateRep) {
    auto tta = ConstructTTAWithAllSymbolsFromIntermediateRep(intermediateRep);
    for(auto& comp : intermediateRep.components) {
        tta.components[comp.name] = {
                .initialLocation           = { comp.initialLocation.isImmediate, comp.initialLocation.identifier },
                .endLocation               = { comp.endLocation.isImmediate, comp.endLocation.identifier },
                .currentLocation           = { comp.initialLocation.isImmediate, comp.initialLocation.identifier },
                .isMain                    = comp.isMain,
                .edges                     = ConvertEdgeListToEdgeMap(comp.edges, tta.GetSymbols(), tta.GetExternalSymbols(), comp.name),
        };
    }
    return tta;
}

TTA TTAParser::ConstructTTAWithAllSymbolsFromIntermediateRep(const TTAIR_t &intermediateRep) {
    TTA tta{};
    for(auto& comp : intermediateRep.components)
        tta.InsertInternalSymbols(ConvertSymbolListToSymbolMap(comp.symbols));
    auto internalSymbols = ConvertSymbolListToSymbolMap(intermediateRep.internalSymbols);
    tta.InsertInternalSymbols(internalSymbols);
    auto externalSymbols = ConvertSymbolListToSymbolMap(intermediateRep.externalSymbols);
    tta.InsertExternalSymbols(externalSymbols);
    return tta;
}

TTA::SymbolMap TTAParser::ConvertSymbolListToSymbolMap(const std::vector<TTAIR_t::Symbol> &symbolList) {
    TTA::SymbolMap map{};
    std::for_each(symbolList.begin(), symbolList.end(), [&map] (auto& symbol) {
        std::visit(overload(
                [&](const int& value)            { map[symbol.identifier] = static_cast<int>(value); },
                [&](const float& value)          { map[symbol.identifier] = static_cast<float>(value); },
                [&](const bool& value)           { map[symbol.identifier] = static_cast<bool>(value); },
                [&](const TTATimerSymbol& value) { map[symbol.identifier] = packToken(value.current_value, PACK_IS_TIMER); },
                [&](const std::string& value)    { map[symbol.identifier] = std::string(value); },
                [&](const auto&& value)          {
                    spdlog::critical("Something went wrong when converting symbol type!");
                    throw std::exception(); }
                ), symbol.value);
    });
    return map;
}

std::unordered_multimap<std::string, TTA::Edge>
TTAParser::ConvertEdgeListToEdgeMap(const std::vector<TTAIR_t::Edge> &edgeList, const TTA::SymbolMap& symbolMap, const TTA::ExternalSymbolMap& externalSymbolMap, const std::string& debugCompName) {
    std::unordered_multimap<std::string, TTA::Edge> edgeMap{};
    calculator calc;
    for(auto& edge : edgeList) {
        // Compile the expressions (guard and update)

        try {
            if (!edge.guardExpression.empty()) {
                calc.compile(edge.guardExpression.c_str(), symbolMap);
                auto type = calc.eval()->type; // We can "safely" eval() guards, because they have no side-effects.
                if(type != tokType_BOOL)
                    spdlog::critical("GuardExpression expression '{0}' is not a boolean expression. It is a '{1}' expression. Component: '{2}'",
                                     edge.guardExpression, static_cast<const tokType>(type), debugCompName.c_str());
            }
        } catch (...) {
            spdlog::critical("Something went wrong when compiling guard expression: '{0}' on component: '{1}'", edge.guardExpression, debugCompName.c_str());
            throw;
        }
        auto updateExpressions = UpdateExpression::ParseExpressions(edge.updateExpression);
        try {
            for(auto& expression : updateExpressions) {
                if(!IsUpdateResettingATimerProperly(expression, symbolMap))
                    spdlog::critical("Update expression '{0} := {1}' updates a variable of type Timer, but doesn't reset it to zero. Component: '{2}'",
                                     expression.lhs, expression.rhs, debugCompName.c_str());
                calc.compile(expression.rhs.c_str(), symbolMap);
            }
        } catch(...) {
            spdlog::critical("Something went wrong when compiling update expression: '{0}' on component: '{1}'", edge.updateExpression, debugCompName.c_str());
            throw;
        }
        edgeMap.insert({edge.sourceLocation.identifier, {
                .sourceLocation           = {edge.sourceLocation.isImmediate,edge.sourceLocation.identifier},
                .targetLocation           = {edge.targetLocation.isImmediate,edge.targetLocation.identifier},
                .guardExpression          = edge.guardExpression, // TODO: Store this as a compiled tree. Strings are nasty
                .externalGuardCollection  = ParseExternalVariablesUsedInGuardExpression(edge.guardExpression, externalSymbolMap),
                .updateExpressions        = updateExpressions
        } });
    }
    return edgeMap;
}
extern void SetVerbosity(unsigned int level);
TTA::GuardCollection TTAParser::ParseExternalVariablesUsedInGuardExpression(const std::string& guardExpression, const TTA::ExternalSymbolMap& externalSymbolMap) {
    SetVerbosity(0); // TODO: We should really do this properly
    if(guardExpression.empty()) return {};
    auto expressions = regex_split(guardExpression, "&&|\\|\\||and|or");
    TTA::GuardCollection coll = {};
    bool doesExpressionContainExternalVariableBool = false;
    auto doesExpressionContainExternalVariable = [&externalSymbolMap, &doesExpressionContainExternalVariableBool](const ASTNode& n) {
        switch (n.type) {
            case NodeType_t::Var:
                doesExpressionContainExternalVariableBool |= externalSymbolMap.find(n.token) != externalSymbolMap.end();
            default: break;
        }
    };
    for(auto& expr : expressions) {
        // TODO: Parentheses fuck everything up. It should be fixed.
        auto ge = ParseGuardExpression(expr);
        if(!ge) continue;
        ge->tree_apply(doesExpressionContainExternalVariable);
        if(doesExpressionContainExternalVariableBool) {
            // Extract the expression only. TODO: Write a more flexible parser
            auto& expressionOnly = ge->children[0].children[0]; // E->c[0](F)->c[0](expression)
            coll.push_back(expressionOnly); // copy into the collection
        }
        doesExpressionContainExternalVariableBool = false;
        delete ge;
    }
    SetVerbosity(1); // TODO: We should really do this properly
    return coll;
}
extern Tree<ASTNode>* ParseQuery(const std::string&);
TTA::GuardExpression* TTAParser::ParseGuardExpression(const std::string &guardExpression) {
    // Reuse the query parser
    std::stringstream ss{};
    ss << "E F " << guardExpression;
    auto query = ParseQuery(ss.str());
    return query;
}

bool TTAParser::IsDocumentAProperPartsFile(const rapidjson::Document &document) {
    if(!JSONParser::DoesMemberExistAndIsArray(document, "parts")) return false;
    auto partsArray = document["parts"].GetArray();
    return std::all_of(partsArray.begin(), partsArray.end(), &IsDocumentAProperPart);
}

bool TTAParser::IsDocumentAProperPart(const rapidjson::Document::ValueType &document) {
    return JSONParser::DoesMemberExistAndIsString(document, "PartName") &&
           JSONParser::DoesMemberExistAndIsString(document, "VariableEnvironment") &&
           IsDocumentAProperExternalType(document["VariableEnvironment"]) &&
           JSONParser::DoesMemberExistAndIsObject(document, "Access") &&
           IsDocumentAProperAccessType(document["Access"]) &&
           JSONParser::DoesMemberExistAndIsObject(document, "GenericType") &&
           IsDocumentAProperGenericType(document["GenericType"]);
}

bool TTAParser::IsDocumentAProperGenericType(const rapidjson::Document::ValueType& document) {
    return  (JSONParser::DoesMemberExistAndIsObject(document, "int") &&
            JSONParser::DoesMemberExistAndIsInt(document["int"], "Value"))
            ||
            (JSONParser::DoesMemberExistAndIsObject(document, "float") &&
            JSONParser::DoesMemberExistAndIsFloat(document["float"], "Value"))
            ||
            (JSONParser::DoesMemberExistAndIsObject(document, "bool") &&
            JSONParser::DoesMemberExistAndIsBool(document["bool"], "Value"))
            ||
            (JSONParser::DoesMemberExistAndIsObject(document, "string") &&
            JSONParser::DoesMemberExistAndIsString(document["string"], "Value"))
            ||
            (JSONParser::DoesMemberExistAndIsObject(document, "Timer") &&
            JSONParser::DoesMemberExistAndIsInt(document["Timer"], "Value"));
}

bool TTAParser::IsDocumentAProperAccessType(const rapidjson::Document::ValueType &document) {
    return JSONParser::DoesMemberExistAndIsBool(document, "Read") && JSONParser::DoesMemberExistAndIsBool(document, "Write");
}

bool TTAParser::IsDocumentAProperExternalType(const rapidjson::Document::ValueType &document) {
    return document.IsString() && (
           document.GetString() == std::string("EXTERNAL") ||
           document.GetString() == std::string("INTERNAL") ||
           document.GetString() == std::string("TIMER")); // TODO: What does Timer mean in terms of external/internal?
}

bool TTAParser::IsDocumentExternalType(const rapidjson::Document::ValueType &document) {
    return document.IsString() && (
            document.GetString() == std::string("EXTERNAL") ||
            document.GetString() == std::string("TIMER"));
}

TTAParser::SymbolExternalPair TTAParser::ParsePart(const rapidjson::Document::ValueType &document) {
    return {.symbol = {.identifier = std::string(document["PartName"].GetString()),
            .value = ParseGenericType(document["GenericType"])},
            .isExternal = IsDocumentExternalType(document["VariableEnvironment"])
    };
}

TTASymbol_t TTAParser::ParseGenericType(const rapidjson::Document::ValueType& document) {
    auto intMember      = document.FindMember("int");
    if(intMember != document.MemberEnd()) return int{ intMember->value["Value"].GetInt() };
    auto floatMember    = document.FindMember("float");
    if(floatMember != document.MemberEnd()) return float{ floatMember->value["Value"].GetFloat() };
    auto boolMember     = document.FindMember("bool");
    if(boolMember != document.MemberEnd()) return bool{ boolMember->value["Value"].GetBool() };
    auto timerMember    = document.FindMember("Timer");
    if(timerMember != document.MemberEnd()) return TTATimerSymbol{static_cast<float>(timerMember->value["Value"].GetInt())};
    auto stringMember   = document.FindMember("string");
    if(stringMember != document.MemberEnd()) return std::string{ stringMember->value["Value"].GetString() };
    spdlog::critical("Error parsing the GenericType. Type is not recognized!");
    return {};
}

bool TTAParser::IsUpdateResettingATimerProperly(const UpdateExpression& expr, const TTA::SymbolMap& context) {
    auto it = context.map().find(expr.lhs);
    if(it != context.map().end() && it->second->type == tokType_TIMER)
        return expr.rhs == "0";
    return true;
}


