#include "da/input_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

static void trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
}

int parseProgramPoint(const std::string &token, bool &isDef, bool &isLastUse) {
    isDef = isLastUse = false;
    if (token.empty())
        throw std::invalid_argument("Empty program-point token");

    char last = token.back();
    std::string numStr = token;
    if (last == '+') {
        isDef = true;
        numStr.pop_back();
    } else if (last == '-') {
        isLastUse = true;
        numStr.pop_back();
    }

    try {
        return std::stoi(numStr);
    } catch (...) {
        throw std::invalid_argument("Invalid program-point token: " + token);
    }
}

std::unordered_map<std::string, std::vector<LiveRange>> parseLiveRanges(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error("Cannot open live-ranges file: " + filename);

    std::unordered_map<std::string, std::vector<LiveRange>> result;
    std::string line;
    int lineNum = 0;

    while (std::getline(fin, line)) {
        ++lineNum;
        trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": missing ':'");

        std::string varName = line.substr(0, colonPos);
        trim(varName);
        if (varName.empty())
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": empty variable name");

        std::string pointsStr = line.substr(colonPos + 1);

        LiveRange lr;
        lr.varName = varName;

        std::istringstream iss(pointsStr);
        std::string token;
        bool firstToken = true;
        while (std::getline(iss, token, ',')) {
            trim(token);
            if (token.empty())
                continue;

            bool isDef = false, isLastUse = false;
            int pt = parseProgramPoint(token, isDef, isLastUse);
            lr.points.push_back(pt);

            if (isDef)
                lr.defPoints.push_back(pt);
            if (isLastUse)
                lr.usePoints.push_back(pt);

            if (firstToken) {
                lr.defFirst = isDef;
                firstToken = false;
            }
            lr.useLast = isLastUse;
        }

        if (lr.points.empty())
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": no program points");

        std::sort(lr.points.begin(), lr.points.end());
        std::sort(lr.defPoints.begin(), lr.defPoints.end());
        std::sort(lr.usePoints.begin(), lr.usePoints.end());

        result[varName].push_back(lr);
    }

    if (result.empty())
        throw std::runtime_error("Live-ranges file is empty or has no valid entries");

    return result;
}

AllocConfig parseConfig(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error("Cannot open config file: " + filename);

    AllocConfig cfg;
    std::string line;
    int lineNum = 0;

    while (std::getline(fin, line)) {
        ++lineNum;
        trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string key   = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        trim(key);
        trim(value);

        if (key == "registers") {
            try {
                cfg.numRegisters = std::stoi(value);
            } catch (...) {
                throw std::runtime_error("Invalid register count: " + value);
            }
            if (cfg.numRegisters < 1)
                throw std::runtime_error("Register count must be >= 1");

        } else if (key == "algorithm") {
            auto commaPos = value.find(',');
            if (commaPos == std::string::npos) {
                cfg.algorithm = value;
                trim(cfg.algorithm);
            } else {
                cfg.algorithm = value.substr(0, commaPos);
                trim(cfg.algorithm);
                std::string paramStr = value.substr(commaPos + 1);
                trim(paramStr);
                try {
                    cfg.algorithmParam = std::stoi(paramStr);
                } catch (...) {
                    throw std::runtime_error("Invalid algorithm parameter: " + paramStr);
                }
            }
        }
    }

    if (cfg.numRegisters == 0)
        throw std::runtime_error("Config file missing 'registers' entry");

    return cfg;
}
