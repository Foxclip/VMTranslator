#include <iostream>
#include <fstream>
#include <string>
#include <cctype>

enum State {
    SPACE,
    SLASH,
    COMMENT,
    INSTR,
    SEGMENT,
    ADDR
};

const int local_pointer = 1;
int compIndex = 0;
std::string outputFilename;

void debugPrint(std::string str) {
    std::cout << str;
}

void debugPrintLine(std::string str) {
    debugPrint(str + '\n');
}

void wAsm(std::string line) {
    std::ofstream stream(outputFilename, std::ios_base::app);
    stream << line.c_str() << std::endl;
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
            wAsm("@" + outputFilename.substr(0, outputFilename.rfind(".")) + "." + address);
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

void writePushPop(std::string instruction, std::string segment, std::string address) {
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
    if(instruction == "push") {
        if(segment == "pointer") {
            if(address == "0") {
                wAsm("@THIS");
            }
            if(address == "1") {
                wAsm("@THAT");
            }
        }
        if(segment == "static") {
            wAsm("@" + outputFilename.substr(0, outputFilename.rfind(".")) + "." + address);
        }
    }
    if(instruction == "push") {
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

void writeNeg(std::string outputFilename) {
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

void writeNot(std::string outputFilename) {
    wAsm("@SP");
    wAsm("A=M");
    wAsm("A=A-1");
    wAsm("M=!M");
}

void writeCommand(std::string insruction, std::string segment, std::string address) {
    debugPrintLine("Writing command: " + insruction + " " + segment + " " + address);
    if(insruction == "push" || insruction == "pop") {
        wAsm("//" + insruction + " " + segment + " " + address);
    } else {
        wAsm("//" + insruction);
    }
    if(insruction == "push" || insruction == "pop") {
        writePushPop(insruction, segment, address);
    }
    if(insruction == "add" || insruction == "sub" || insruction == "and" || insruction == "or") {
        writeAddSubAndOr(insruction);
    }
    if(insruction == "neg") {
        writeNeg(outputFilename);
    }
    if(insruction == "eq" || insruction == "gt" || insruction == "lt") {
        writeComp(insruction, compIndex);
        compIndex++;
    }
    if(insruction == "not") {
        writeNot(outputFilename);
    }
}

void translate(std::string inputFilename) {
    std::cout << "Translating " + inputFilename << std::endl;
    std::fstream inputStream(inputFilename);
    if(inputStream.bad()) {
        std::cout << "Error" << std::endl;
    }
    outputFilename = inputFilename.substr(0, inputFilename.rfind(".")) + ".asm";
    std::cout << "Output file: " + outputFilename << std::endl;
    std::ofstream clearFileContents(outputFilename, std::ios::trunc);
    clearFileContents.close();
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
            } else if(currentState == INSTR) {
                writeCommand(instrBuffer, segmentBuffer, addressBuffer);
                currentState = SPACE;
            }
        }
        if(c == '/') {
            debugPrintLine("Slash symbol found");
            if(currentState == SLASH) {
                debugPrintLine("Comment found");
                currentState = COMMENT;
            }
            else if(currentState == SPACE) {
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
        if(std::isalpha(c)) {
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

int main(int argc, char *argv[]) {

    translate(argv[1]);

    return 0;

}