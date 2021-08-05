#include "utils.h"

float StringToNum(string str){
  float tmp = 0;
  tmp = stof(str, NULL);
  return tmp;
}

PIM_OPERATION StringToPIM_OP(string str) {
    std::cout << std::endl << str << std::endl;
    if(str == "ADD") return PIM_OPERATION::ADD;
    else if(str == "MUL") return PIM_OPERATION::MUL;
    else if(str == "MAC") return PIM_OPERATION::MAC;
    else if(str == "MAD") return PIM_OPERATION::MAD;
    else if(str == "ADD_AAM") return PIM_OPERATION::ADD_AAM;
    else if(str == "MUL_AAM") return PIM_OPERATION::MUL_AAM;
    else if(str == "MAC_AAM") return PIM_OPERATION::MAC_AAM;
    else if(str == "MAD_AAM") return PIM_OPERATION::MAD_AAM;
    else if(str == "MOV") return PIM_OPERATION::MOV;
    else if(str == "FILL") return PIM_OPERATION::FILL;
    else if(str == "NOP") return PIM_OPERATION::NOP;
    else if(str == "JUMP") return PIM_OPERATION::JUMP;
    else if(str == "EXIT") return PIM_OPERATION::EXIT;
    else return PIM_OPERATION::NOP;
}

PIM_OPERAND StringToOperand(string str) {
    if (str.find("ODD_BANK") != std::string::npos)
        return PIM_OPERAND::ODD_BANK;
    else if (str.find("EVEN_BANK") != std::string::npos)
        return PIM_OPERAND::EVEN_BANK;
    else if (str.find("GRF_A") != std::string::npos)
        return PIM_OPERAND::GRF_A;
    else if (str.find("GRF_B") != std::string::npos)
        return PIM_OPERAND::GRF_B;
    else if (str.find("SRF_A") != std::string::npos)
        return PIM_OPERAND::SRF_A;
    else if (str.find("SRF_M") != std::string::npos)
        return PIM_OPERAND::SRF_M;
    else
        return PIM_OPERAND::NONE;
}

int StringToIndex(string str) {
    if (str.find("0") != std::string::npos)
        return 0;
    else if (str.find("1") != std::string::npos)
        return 1;
    else if (str.find("2") != std::string::npos)
        return 2;
    else if (str.find("3") != std::string::npos)
        return 3;
    else if (str.find("4") != std::string::npos)
        return 4;
    else if (str.find("5") != std::string::npos)
        return 5;
    else if (str.find("6") != std::string::npos)
        return 6;
    else if (str.find("7") != std::string::npos)
        return 7;
    else
        return -1;
}
