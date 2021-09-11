#include "utils.h"

float StringToNum(std::string str) {
  float tmp = 0;
  tmp = stof(str, NULL);
  return tmp;
}

PIM_OPERATION StringToPIM_OP(std::string str) {
    // std::cout << std::endl << str << std::endl;
    if (str == "ADD" || str == "ADD_AAM")
        return PIM_OPERATION::ADD;
    else if (str == "MUL" || str == "MUL_AAM")
        return PIM_OPERATION::MUL;
    else if (str == "MAC" || str == "MAC_AAM")
        return PIM_OPERATION::MAC;
    else if (str == "MAD" || str == "MAD_AAM")
        return PIM_OPERATION::MAD;
    else if (str == "MOV")
        return PIM_OPERATION::MOV;
    else if (str == "FILL")
        return PIM_OPERATION::FILL;
    else if (str == "NOP")
        return PIM_OPERATION::NOP;
    else if (str == "JUMP")
        return PIM_OPERATION::JUMP;
    else if (str == "EXIT")
        return PIM_OPERATION::EXIT;
    else
        return PIM_OPERATION::NOP;
}

bool CheckAam(std::string str) {
    if (str.find("AAM") != std::string::npos)
        return true;
    else
        return false;
}

PIM_OPERAND StringToOperand(std::string str) {
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

int StringToIndex(std::string str) {
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
