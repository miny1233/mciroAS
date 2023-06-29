#include <iostream>
#include <map>
#include <format>
#include <utility>
#include <vector>
#include <functional>
#include <cstring>
#include <bitset>
#include <fstream>
#include <cassert>
#include <csignal>

struct binary
{
    uint8_t end:2;
    uint8_t first:2;
    uint8_t op:4;
    uint8_t extend :8;
}__attribute__((packed));

class BuildErrorMsg : public std::exception {
private:
    std::string error_msg;
public:
    BuildErrorMsg() = delete;
    explicit BuildErrorMsg(std::string error):error_msg(std::move(error)){}
    [[nodiscard]] const char * what() const noexcept override{
        return error_msg.c_str();
    }
};

class StateMachine
{
protected:
    binary command{};
    size_t line;
    struct RegisterList {
        std::map<std::string,int> reg_map{
                {"r0",0b00},
                {"r1",0b01},
                {"r2",0b10},
                {"r3",0b11},
        };
        unsigned char operator[](std::string regname)
        {
            if(reg_map.find(regname) == reg_map.end())throw BuildErrorMsg(std::format("Unknown Register Name '{}'",regname));
            return reg_map[regname];
        }
    }regmap;
public:
    bool extend_enable = false;
    explicit StateMachine(size_t line) : line(line){}
    virtual binary translate(std::string args) = 0;
    std::vector<std::string> support;    //支持的指令
    virtual ~StateMachine() = default;
};

class RR_Type : public StateMachine
{
private:
    std::map<std::string,int> opmap{
            {"mov",0b0100},
            {"add",0b0000},
            {"sub",0b1000},
            {"and",0b0001},
            {"or",0b1001},
            {"rr",0b1010},
            {"inc",0b0111}
    };
    std::string opn;
public:
    RR_Type(std::string& OpName,size_t line): StateMachine(line),opn(OpName)
    {
        if(opmap.find(OpName) == opmap.end())throw BuildErrorMsg("Unknown Operator");
        command.op = opmap[OpName];
        extend_enable = false;
    }
    binary translate(std::string args) override
    {
        char temp[64];
        strcpy(temp,args.c_str());
        char* rd_c = strtok(temp,","),*rs_c = strtok(nullptr,",");
        if(opn != "inc")command.first = regmap[rs_c];
        command.end = regmap[rd_c];
        return command;
    }
};

class RS_Type : public StateMachine{
private:
    std::map<std::string,int> opmap{
            {"lad",0b1100},
            {"sta",0b1101},
            {"jmp",0b1110},
            {"bzc",0b1111},
    };
public:
    RS_Type(std::string& OpName,size_t line) : StateMachine(line)
    {
        if(opmap.find(OpName) == opmap.end())throw BuildErrorMsg("Unknown Operator");
        command.op = opmap[OpName];
        extend_enable = false;
    }
    binary translate(std::string args) override
    {
        char temp[64];
        strcpy(temp,args.c_str());
        char* find_mem = strtok(temp," ,"), \
        *d = strtok(nullptr," ,"), \
        *r = strtok(nullptr," ,");
        command.first = *find_mem - '0';  // 要求用数字代表寻址方式
        command.end = r == nullptr ? 0 : regmap[r];
        int address;
        if(sscanf(d,"%x",&address) != 1)throw BuildErrorMsg("Address Format Error");
        command.extend = address;
        extend_enable = true;
        return command;
    }
};

class IO_type : public StateMachine{
private:
    std::map<std::string,int> opmap{
            {"in",0b0010},
            {"out",0b0011},
            {"ldi",0b0110}
    };
    bool convert; // 交换操作数
public:
    IO_type(std::string& OpName,size_t line) : StateMachine(line)
    {
        if(opmap.find(OpName) == opmap.end())throw BuildErrorMsg("Unknown Operator");
        command.op = opmap[OpName];
        extend_enable = false;
        convert = (OpName == "out");
    }
    binary translate(std::string args) override{
        char temp[64],*p,*r;
        strcpy(temp,args.c_str());
        if(!convert) {
            r = strtok(temp, ",");
            p = strtok(nullptr, ",");
            command.end = regmap[r];
        }else {
            p = strtok(temp, ",");
            r = strtok(nullptr, ",");
            command.first = regmap[r];
        }
        int address;
        if(sscanf(p,"%x",&address) != 1)throw BuildErrorMsg("Address Format Error");
        command.extend = address;
        extend_enable = true;
        return command;
    }
};
class Other_Type :public StateMachine{
private:
    std::map<std::string,int> opmap{
            {"halt",0b0101}
    };
public:
    Other_Type(std::string& OpName,size_t line) : StateMachine(line)
    {
        if(opmap.find(OpName) == opmap.end())throw BuildErrorMsg("Unknown Operator");
        command.op = opmap[OpName];
        extend_enable = false;
    }
    binary translate(std::string args) override{
        return command;
    }
};
StateMachine* RRMachine(std::string op,size_t line){return new RR_Type(op,line);}
StateMachine* RSMachine(std::string op,size_t line){return new RS_Type(op,line);}
StateMachine* IOMachine(std::string op,size_t line){return new IO_type(op,line);}
StateMachine* OTMachine(std::string op,size_t line){return new Other_Type(op,line);}

size_t code_line = 0;
void building_error(int sign) noexcept
{
    if(sign == SIGSEGV)std::cerr<<"unknown mistake"<<std::endl;
    std::cerr << "at line "<<code_line<<std::endl;
    exit(-1);
}

int main(int argc,char *argv[]) {
    std::map<std::string,std::function<StateMachine*(std::string,size_t)>> supportList{
            //RR
            {"mov", RRMachine},{"add",RRMachine},
            {"sub",RRMachine},{"and",RRMachine},
            {"or",RRMachine},{"rr",RRMachine},
            {"inc",RRMachine},
            //RS
            {"lad", RSMachine},{"sta",RSMachine},
            {"jmp",RSMachine},{"bzc",RSMachine},
            //IO
            {"in", IOMachine},{"out", IOMachine},
            {"ldi", IOMachine},
            //other
            {"halt", OTMachine}
    };

    assert(argc >= 2);
    signal(SIGSEGV,building_error);
    std::string path = argv[1];
    std::fstream code,out;
    code.open(path);
    out.open("./build.txt",std::ios_base::out);
    if(!code.is_open())throw BuildErrorMsg("Cannot Open Source File");
    std::string command;
    size_t line = 0;
    while(getline(code,command)) {
        code_line++;
        if ([&command]() -> bool {  //判断是否为空行
            for(auto ch:command)if(ch != ' ')return true;
            return false;
        }()) {
            try {
                std::vector<char> temp(64);  //缓冲区 strtok需要修改源字符串
                strcpy(&temp[0], command.c_str());
                std::string op = strtok(&temp[0], " ");
                char *arg = strtok(nullptr, "");
                std::string other = arg == nullptr ? std::string() : arg;
                if(supportList.find(op) == supportList.end())throw BuildErrorMsg(std::format("Unknown Operator '{}'" ,op));
                auto machine = supportList[op](op, code_line);
                auto ret = machine->translate(other);
                out << std::format("$P {0:02X} {1:02X} ;{2}\n", line++, reinterpret_cast<uint8_t *>(&ret)[0], command);
                if (machine->extend_enable)
                    out << std::format("$P {0:02X} {1:02X}\n", line++, reinterpret_cast<uint8_t *>(&ret)[1]);
                delete machine;
            }catch (std::exception& e){
                std::cerr << "error: "<<e.what()<<"\n";
                building_error(1);
            }
        }
    }
}
