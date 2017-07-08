#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <vector>
#include "dirent.h"

enum State {
    SPACE,
    SLASH,
    COMMENT,
    INSTR,
    SEGMENT,
    ADDR
};

const int local_pointer = 1;
int runningIndex = 0;
int lineNumber = 0;
std::string outputFilename;
std::string currentInputFilename;

void debugPrint(std::string str) {
    //std::cout << str;
}

void debugPrintLine(std::string str) {
    debugPrint(str + '\n');
}

void wAsm(std::string line) {
    std::ofstream stream(outputFilename, std::ios_base::app);
    stream << line.c_str() << std::endl;
    if(line[0] != '/' && line[0] != '(') {
        lineNumber++;
    }
}

void writePush(std::string segment, std::string address) {  
    if(segment != "constant" && segment != "pointer" && segment != "static") {
        if(segment != "temp") {
            wAsm("A=M");
        }
        wAsm("D=A");
        wAsm("@" + address);
        wAsm("A=D+A");
        wAsm("A=M");
    }
    if(segment == "pointer" || segment == "static") {
        wAsm("A=M");
    }
    wAsm("D=A");
    wAsm("@SP");
    wAsm("A=M");
    wAsm("M=D");
    wAsm("@SP");
    wAsm("M=M+1");
}

void writePop(std::string segment, std::string address) {
    if(segment == "pointer" || segment == "static") {
        wAsm("@SP");
        wAsm("M=M-1");
        wAsm("A=M");
        wAsm("D=M");
        if(segment == "pointer") {
            if(address == "0") {
                wAsm("@THIS");
            }
            if(address == "1") {
                wAsm("@THAT");
            }
        }
        if(segment == "static") {
            wAsm("@" + currentInputFilename + "." + address);
        }
        wAsm("M=D");
        return;
    }
    if(segment == "temp") {
        wAsm("D=A");
    } else {
        wAsm("D=M");
    }
    wAsm("@" + address);
    wAsm("D=D+A");
    wAsm("@SP");
    wAsm("A=M");
    wAsm("M=D");
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("D=M");
    wAsm("A=A+1");
    wAsm("A=M");
    wAsm("M=D");
    wAsm("@SP");
    wAsm("M=M-1");
}

void writePushPop(std::string command, std::string segment, std::string address) {
    if(segment == "constant") {
        wAsm("@" + address);
    }
    if(segment == "local") {
        wAsm("@LCL");
    }
    if(segment == "argument") {
        wAsm("@ARG");
    }
    if(segment == "this") {
        wAsm("@THIS");
    }
    if(segment == "that") {
        wAsm("@THAT");
    }
    if(segment == "temp") {
        wAsm("@5");
    }
    if(command == "push") {
        if(segment == "pointer") {
            if(address == "0") {
                wAsm("@THIS");
            }
            if(address == "1") {
                wAsm("@THAT");
            }
        }
        if(segment == "static") {
            wAsm("@" + currentInputFilename + "." + address);
        }
    }
    if(command == "push") {
        writePush(segment, address);
    } else {
        writePop(segment, address);
    }
}

void writeAddSubAndOr(std::string command) {
    wAsm("@SP");
    wAsm("M=M-1");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("D=M");
    wAsm("A=A+1");
    if(command == "add") {
        wAsm("D=D+M");
    }
    if(command == "sub") {
        wAsm("D=D-M");
    }
    if(command == "and") {
        wAsm("D=D&M");
    }
    if(command == "or") {
        wAsm("D=D|M");
    }
    wAsm("A=A-1");
    wAsm("M=D");
}

void writeNeg() {
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("D=0");
    wAsm("M=D-M");
}

void writeComp(std::string command, int index) {
    wAsm("@SP");
    wAsm("M=M-1");
    wAsm("A=M");
    wAsm("D=M");
    wAsm("A=A-1");
    wAsm("A=M");
    wAsm("D=D-A");
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("M=0");
    if(command == "eq") {
        wAsm("@EQ" + std::to_string(index));
        wAsm("D;JNE");
    }
    if(command == "gt") {
        wAsm("@GT" + std::to_string(index));
        wAsm("D;JGE");
    }
    if(command == "lt") {
        wAsm("@LT" + std::to_string(index));
        wAsm("D;JLE");
    }
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("M=-1");
    if(command == "eq") {
        wAsm("(EQ" + std::to_string(index) + ")");
    }
    if(command == "gt") {
        wAsm("(GT" + std::to_string(index) + ")");
    }
    if(command == "lt") {
        wAsm("(LT" + std::to_string(index) + ")");
    }
}

void writeNot() {
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("M=!M");
}

void writeLabel(std::string labelName) {
    wAsm("(" + labelName + ")");
}

void writeIfGoto(std::string label) {
    wAsm("@SP");
    wAsm("MD=M-1");
    wAsm("A=M");
    wAsm("D=M");
    wAsm("@" + label);
    wAsm("D;JNE");
}

void writeGoto(std::string label) {
    wAsm("@" + label);
    wAsm("0;JMP");
}

void writeFunction(std::string name, int localVarCount) {
    writeLabel(name);
    for(int i = 0; i < localVarCount; i++) {
        writePushPop("push", "constant", "0");
    }
}

void writeFramePart(std::string address) {
    wAsm("@endFrame");
    wAsm("M=M-1");
    wAsm("A=M");
    wAsm("D=M");
    wAsm("@" + address);
    wAsm("M=D");
}

void writeReturn() {
    wAsm("@LCL");
    wAsm("D=M");
    wAsm("@5");
    wAsm("D=D-A");
    wAsm("A=D");
    wAsm("D=M");
    wAsm("@retAddr");
    wAsm("M=D");
    writePushPop("pop", "argument", "0");
    wAsm("@LCL");
    wAsm("D=M");
    wAsm("@endFrame");
    wAsm("M=D");
    wAsm("@ARG");
    wAsm("D=M+1");
    wAsm("@SP");
    wAsm("M=D");
    writeFramePart("THAT");
    writeFramePart("THIS");
    writeFramePart("ARG");
    writeFramePart("LCL");
    wAsm("@retAddr");
    wAsm("A=M");
    wAsm("0;JMP");
}

void writeShortPush(std::string str) {
    wAsm("@" + str);
    wAsm("D=M");
    wAsm("@SP");
    wAsm("A=M");
    wAsm("M=D");
    wAsm("@SP");
    wAsm("M=M+1");
}

void writeCall(std::string function, int argCount) {
    writePushPop("push", "constant", function + "$ret." + std::to_string(runningIndex));
    writeShortPush("LCL");
    writeShortPush("ARG");
    writeShortPush("THIS");
    writeShortPush("THAT");
    wAsm("@SP");
    wAsm("D=M");
    wAsm("@5");
    wAsm("D=D-A");
    wAsm("@" + std::to_string(argCount));
    wAsm("D=D-A");
    wAsm("@ARG");
    wAsm("M=D");
    wAsm("@SP");
    wAsm("D=M");
    wAsm("@LCL");
    wAsm("M=D");
    writeGoto(function);
    writeLabel(function + "$ret." + std::to_string(runningIndex));
}

void writeCommand(std::string command, std::string segment, std::string address) {
    debugPrintLine("Writing command: " + command + " " + segment + " " + address);
    if(command == "push" || command == "pop" || command == "function" || command == "call") {
        wAsm("//" + command + " " + segment + " " + address);
    } else if(command == "label" || command == "if-goto" || command == "goto") {
        wAsm("//" + command + " " + segment);
    } else {
        wAsm("//" + command);
    }
    if(command == "push" || command == "pop") {
        writePushPop(command, segment, address);
    }
    if(command == "add" || command == "sub" || command == "and" || command == "or") {
        writeAddSubAndOr(command);
    }
    if(command == "neg") {
        writeNeg();
    }
    if(command == "eq" || command == "gt" || command == "lt") {
        writeComp(command, runningIndex);
    }
    if(command == "not") {
        writeNot();
    }
    if(command == "label") {
        writeLabel(segment);
    }
    if(command == "if-goto") {
        writeIfGoto(segment);
    }
    if(command == "goto") {
        writeGoto(segment);
    }
    if(command == "function") {
        writeFunction(segment, std::stoi(address));
    }
    if(command == "return") {
        writeReturn();
    }
    if(command == "call") {
        writeCall(segment, std::stoi(address));
    }
    runningIndex++;
}

void writeBootstrap() {
    wAsm("@256");
    wAsm("D=A");
    wAsm("@SP");
    wAsm("M=D");
    writeCall("Sys.init", 0);
}

void translate(std::string inputFilename) {
    std::cout << "Translating " + inputFilename << std::endl;
    std::fstream inputStream(inputFilename);
    if(inputStream.bad()) {
        std::cout << "Error" << std::endl;
    }
    std::string instrBuffer;
    std::string segmentBuffer;
    std::string addressBuffer;
    char c;
    State currentState = SPACE;
    while(inputStream >> std::noskipws >> c) {
        debugPrint("State: ");
        switch(currentState) {
            case SPACE:         debugPrintLine("space");        break;
            case SLASH:         debugPrintLine("slash");        break;
            case COMMENT:       debugPrintLine("comment");      break;
            case INSTR:         debugPrintLine("instr");        break;
            case SEGMENT:       debugPrintLine("segment");      break;
            case ADDR:          debugPrintLine("addr");         break;
        }
        if(c == '\n') {
            debugPrintLine("Read symbol \\n");
        }
        else {
            debugPrintLine(std::string("Read symbol ") + c);
        }
        if(c == '\n') {
            if(currentState == COMMENT) {
                currentState = SPACE;
            } else if(currentState == INSTR || currentState == SEGMENT) {
                writeCommand(instrBuffer, segmentBuffer, addressBuffer);
                currentState = SPACE;
            }
        }
        if(c == '/') {
            debugPrintLine("Slash symbol found");
            if(currentState == SLASH) {
                debugPrintLine("Comment found");
                currentState = COMMENT;
            } else if(currentState == SPACE) {
                currentState = SLASH;
            } else if(currentState == ADDR) {
                writeCommand(instrBuffer, segmentBuffer, addressBuffer);
                currentState = SLASH;
            }
        }
        if(std::isspace(c)) {
            debugPrintLine("Whitespace symbol found");
            if(currentState == INSTR) {
                currentState = SEGMENT;
            } else if(currentState == SEGMENT) {
                currentState = ADDR;
            } else if(currentState == ADDR) {
                writeCommand(instrBuffer, segmentBuffer, addressBuffer);
                currentState = SPACE;
            }
        }
        if(std::isalnum(c) || c == '-' || c == '_' || c == '.') {
            debugPrintLine("Letter symbol found");
            if(currentState == INSTR) {
                instrBuffer += c;
                debugPrintLine("instrBuffer: " + instrBuffer);
            } else if(currentState == SEGMENT) {
                segmentBuffer += c;
                debugPrintLine("segmentBuffer: " + segmentBuffer);
            } else if(currentState == SPACE) {
                currentState = INSTR;
                instrBuffer.clear();
                segmentBuffer.clear();
                addressBuffer.clear();
                instrBuffer += c;
                debugPrintLine("instrBuffer: " + instrBuffer);
            }
        }
        if(std::isdigit(c)) {
            if(currentState == ADDR) {
                addressBuffer += c;
                debugPrintLine("addressBuffer: " + addressBuffer);
            }
        }

        debugPrintLine("");

    }
}

void setOutputFile(std::string name) {
    outputFilename = name + ".asm";
    std::cout << "Output file: " + outputFilename << std::endl;
    std::ofstream clearFileContents(outputFilename, std::ios::trunc);
    clearFileContents.close();
}

bool endsWith(std::string const &fullString, std::string const &ending) {
    if(fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

int main(int argc, char *argv[]) {

    std::string inputName(argv[1]);
    if(inputName.back() == '/' || inputName.back() == '\\') {
        DIR *dir;
        struct dirent *ent;
        std::string dirName = inputName.substr(0, inputName.size() - 1);
        if((dir = opendir(dirName.c_str())) != NULL) {
            setOutputFile(inputName + dirName);
            writeBootstrap();
            while((ent = readdir(dir)) != NULL) {
                if(ent->d_type == DT_REG && endsWith(ent->d_name, ".vm")) {
                    currentInputFilename = std::string(ent->d_name);
                    currentInputFilename = currentInputFilename.substr(0, currentInputFilename.rfind("."));
                    translate(inputName + ent->d_name);
                }
            }
            closedir(dir);
        } else {
            perror("Error");
        }
    } else {
        setOutputFile(inputName);
        writeBootstrap();
        currentInputFilename = inputName;
        translate(inputName);
    }

    return 0;

}