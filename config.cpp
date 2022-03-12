#include <fstream>
#include <regex>
#include <sstream>

#include <Windows.h>

#include "config.h"

class NotesConfigParseException : std::exception {
private:
    std::string msg_;
public:
    NotesConfigParseException(const std::string& msg) : msg_(msg) {}

    virtual const char* what() const throw () {
        return msg_.c_str();
   }
};

NotesConfig::NotesConfig(const std::string& notesFile) {
    std::ifstream file(notesFile);
    std::string line;
    while(std::getline(file, line)) {
        if(!tryParseLine(line)) {
            throw NotesConfigParseException("failed to parse line: " + line);
        }
    }
}

char parseHotkeyBaseString(const std::string& s) {
    std::regex r("hotkeyBase \'(.)\'");
    std::cmatch m;
    if(!std::regex_match(s.c_str(), m, r)) {
        return false;
    }
    std::string match = m[1];
    if(match.length() != 1) {
        return '\0';
    }
    return match[0];
}

bool NotesConfig::tryParseLine(const std::string& line) {
    std::istringstream iss(line);
    std::string fieldName;
    std::string fieldVal;
    iss >> fieldName;

    if(fieldName == "notesFileName") {
        iss >> notesFileName_;
    } else if(fieldName == "windowWidth") {
        iss >> windowWidth_;
    } else if(fieldName == "windowHeight") {
        iss >> windowHeight_;
    } else if(fieldName == "hotkeyBase") {
        hotkeyBase_ = parseHotkeyBaseString(line);
        if(hotkeyBase_ == '\0') {
            return false; 
        }
    } else if(fieldName == "hotkeyMod") { 
        std::string hotkeyModString;
        iss >> hotkeyModString;
        if(hotkeyModString == "CTRL") {
            hotkeyMod_ = MOD_CONTROL;
        } else if(hotkeyModString == "ALT") {
            hotkeyMod_ = MOD_ALT;
        } else if(hotkeyModString == "SHIFT") {
            hotkeyMod_ = MOD_SHIFT;
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}
    
    std::string NotesConfig::getNotesFileName() {
        return notesFileName_;
    }
    int NotesConfig::getWindowWidth() {
        return windowWidth_;
    }
    int NotesConfig::getWindowHeight() {
        return windowHeight_;
    }
    int NotesConfig::getHotkeyBase() {
        return hotkeyBase_;
    }
    int NotesConfig::getHotkeyMod() {
        return hotkeyMod_;
    }