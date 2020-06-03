#include <iostream>
#include <vector>
#include <map>
#include <cmath>

//parser

enum instr_t{
    INSTR_EOF,
    //input and output
    INSTR_IO,
    INSTR_IO_IC,//input character into heap in stack position
    INSTR_IO_IN,//input number into heap in stack position
    INSTR_IO_OC,//output character from stack position in heap
    INSTR_IO_ON,//output number from stack position in heap
    //stack manipulation
    INSTR_STACK,
    INSTR_STACK_PSH,//read number from code and push into stack
    INSTR_STACK_DUPN,//duplicate stack value
    INSTR_STACK_POPN,//pop stack and discard
    INSTR_STACK_DUP,//duplicate stack value
    INSTR_STACK_SWP,//swap stack value
    INSTR_STACK_POP,//pop stack and discard
    //arithmetic
    INSTR_MATH,
    INSTR_MATH_ADD,
    INSTR_MATH_SUB,
    INSTR_MATH_MUL,
    INSTR_MATH_DIV,
    INSTR_MATH_MOD,
    //flow control
    INSTR_FC,
    INSTR_FC_LBL,//label
    INSTR_FC_CSR,//call subroutine
    INSTR_FC_JMP,//jump
    INSTR_FC_JZ,//jump if stack top is zero
    INSTR_FC_JN,//jump if stack top is negative
    INSTR_FC_RET,//return from subroutine
    INSTR_FC_END,//end execution
    //heap access
    INSTR_MEM,
    INSTR_MEM_STORE,//store in heap at stack value
    INSTR_MEM_LOAD,//get value from heap at stack value
    INSTR_INVALID,
};

struct Parser{
    const std::string &data;
    uint32_t i;
    uint32_t unsolved_labels;
    std::map<std::string,uint32_t> labels;
    std::map<std::string,std::vector<uint32_t>> jumps;
    std::vector<uint32_t> code;

    Parser(const std::string &s):data(s),i(0),unsolved_labels(0){
    }

    char next(){
        if(i>=data.size())return 0;
        while(data[i]!=' '&&data[i]!='\t'&&data[i]!='\n'){
            i++;
            if(i>=data.size())return 0;
        }
        return data[i++];
    }

    void add_instr(instr_t i){
        code.push_back(i);
    }

    void add_stack(instr_t i,int32_t val){
        code.push_back(i);
        code.push_back(*reinterpret_cast<uint32_t*>(&val));//store the integer's bytes
    }

    void add_jump(instr_t i,std::string label){
        code.push_back(i);
        if(labels.find(label)!=labels.end()){
            code.push_back(labels[label]);
        }else{
            uint32_t pos=get_top();
            code.push_back(0);//temporary destination
            if(jumps.find(label)!=jumps.end()){
                jumps[label].push_back(pos);
            }else{
                unsolved_labels++;
                jumps[label]=std::vector<uint32_t>({pos});
            }
        }
    }

    void add_label(std::string label){
        if(labels.find(label)!=labels.end())throw std::runtime_error("label redefinition");
        uint32_t pos=get_top();
        labels[label]=pos;
        if(jumps.find(label)!=jumps.end()){
            unsolved_labels--;
            for(uint32_t i:jumps[label]){
                code[i]=pos;//rewrite temporary destinations with actual destination
            }
        }
    }

    uint32_t get_top(){
        return code.size();
    }
};

enum imp_t{
    IMP_EOF,
    IMP_IO,
    IMP_STACK,
    IMP_MATH,
    IMP_FC,
    IMP_MEM,
};

imp_t read_imp(Parser &p){
    switch(p.next()){
    case 0: return IMP_EOF;
    case ' ': return IMP_STACK;
    case '\t':
        switch(p.next()){
        case ' ': return IMP_MATH;
        case '\t': return IMP_MEM;
        case '\n': return IMP_IO;
        }
    case '\n': return IMP_FC;
    }
    throw std::runtime_error("unreachable");
}

instr_t read_instr(Parser &p){
    switch(read_imp(p)){
    case IMP_EOF:return INSTR_EOF;//propagate EOF
    case IMP_IO:
        switch(p.next()){
        case '\t':
            switch(p.next()){
            case ' ': return INSTR_IO_IC;
            case '\t': return INSTR_IO_IN;
            default:
                throw std::runtime_error("unknown IO input instruction");
            }
        case ' ':
            switch(p.next()){
            case ' ': return INSTR_IO_OC;
            case '\t': return INSTR_IO_ON;
            default:
                throw std::runtime_error("unknown IO output instruction");
            }
        default:
            throw std::runtime_error("unknown IO instruction");
        }
    case IMP_STACK:
        switch(p.next()){
        case ' ': return INSTR_STACK_PSH;
        case '\t'://seems to be a nonstandard extension?
            switch(p.next()){
            case ' ': return INSTR_STACK_DUPN;
            case '\n': return INSTR_STACK_POPN;
            }
        case '\n':
            switch(p.next()){
            case ' ': return INSTR_STACK_DUP;
            case '\t': return INSTR_STACK_SWP;
            case '\n': return INSTR_STACK_POP;
            }
        default:
            throw std::runtime_error("unknown STACK instruction");
        }
    case IMP_MATH:
        switch(p.next()){
        case ' ':
            switch(p.next()){
            case ' ': return INSTR_MATH_ADD;
            case '\t': return INSTR_MATH_SUB;
            case '\n': return INSTR_MATH_MUL;
            }
        case '\t':
            switch(p.next()){
            case ' ': return INSTR_MATH_DIV;
            case '\t': return INSTR_MATH_MOD;
            default:
                break;
            }
        default:
            throw std::runtime_error("unknown MATH instruction");
        }
    case IMP_FC:
        switch(p.next()){
        case ' ':
            switch(p.next()){
            case ' ': return INSTR_FC_LBL;
            case '\t': return INSTR_FC_CSR;
            case '\n': return INSTR_FC_JMP;
            }
        case '\t':
            switch(p.next()){
            case ' ': return INSTR_FC_JZ;
            case '\t': return INSTR_FC_JN;
            case '\n': return INSTR_FC_RET;
            }
        case '\n':
            if(p.next()=='\n') return INSTR_FC_END;
            throw std::runtime_error("unknown FC instruction");
        }
    case IMP_MEM:
        switch(p.next()){
        case ' ': return INSTR_MEM_STORE;
        case '\t': return INSTR_MEM_LOAD;
        default:
            throw std::runtime_error("unknown MEM instruction");
        }
    }
    throw std::runtime_error("unknown instruction");
}

std::string read_label(Parser &p){
    std::string s;
    while(true){
        switch(p.next()){
        case ' ':
            s+='0';
            break;
        case '\t':
            s+='1';
            break;
        case '\n':
            return s;
        }
    }
}

uint32_t read_number(Parser &p,uint8_t maxbits){//doesn't read the sign
    std::string s;
    while(true){
        if(s.size()>maxbits)throw std::runtime_error("number too large");
        switch(p.next()){
        case ' ':
            s+='0';
            break;
        case '\t':
            s+='1';
            break;
        case '\n':
            return s==""?0:std::stoul(s,nullptr,2);
        }
    }
}

bool save_instruction(Parser &p,instr_t i){
    if(i==INSTR_EOF)return true;//end of file, stop parsing
    switch(i){
    case INSTR_STACK_PSH:
    case INSTR_STACK_DUPN:
    case INSTR_STACK_POPN:
        {
            bool negative;
            switch(p.next()){
            case ' ':
                negative=false;
                break;
            case '\t':
                negative=true;
                break;
            case '\n':
                throw std::runtime_error("malformed number");
            }
            int32_t n=static_cast<int32_t>(read_number(p,31));
            p.add_stack(i,negative?-n:n);
            return false;
        }
    case INSTR_FC_LBL:
    case INSTR_FC_CSR:
    case INSTR_FC_JMP:
    case INSTR_FC_JZ:
    case INSTR_FC_JN:
        {
            std::string lbl=read_label(p);
            if(i==INSTR_FC_LBL){
                p.add_label(lbl);
            }else{
                p.add_jump(i,lbl);
            }
            return false;
        }
    default:
        p.add_instr(i);
        return false;
    }
}

std::vector<uint32_t> parse(const std::string &data){
    Parser p(data);
    while(!save_instruction(p,read_instr(p)));
    if(p.code.size()==0)throw std::runtime_error("no code");
    if(p.unsolved_labels>0)throw std::runtime_error("jump to undefined label");
    return std::move(p.code);
}

//vm

struct output_handler{
    virtual void put_c(char)=0;
    virtual void put_i(int)=0;
};

struct input_provider{
    virtual char next_c()=0;
    virtual int next_i()=0;
};

constexpr double knuth_mod(double a,double b){
    return a - b * floor(a/b);
}

struct VM{
    VM(std::vector<uint32_t> && d,input_provider &in,output_handler &out):data(d),input(in),output(out),pc(0){
    }
    std::vector<uint32_t> data;
    input_provider &input;
    output_handler &output;
    uint32_t pc;
    std::vector<int32_t> stack;
    std::vector<uint32_t> call_stack;
    std::map<int32_t,int32_t> heap;
    int32_t pop(){
        if(stack.size()==0) throw std::runtime_error("stack undeflow");
        int32_t i=stack.back();
        stack.pop_back();
        return i;
    }
    bool step(){
        instr_t i=next();
        switch(i){
        case INSTR_IO_IC:{
                int32_t addr=pop();
                heap[addr]=input.next_c();
            }
            break;
        case INSTR_IO_IN:{
                int32_t addr=pop();
                heap[addr]=input.next_i();
            }
            break;
        case INSTR_IO_OC:
            output.put_c(pop());
            break;
        case INSTR_IO_ON:
            output.put_i(pop());
            break;
        case INSTR_STACK_PSH:{
                stack.push_back(*reinterpret_cast<int32_t*>(&data[pc++]));
            }
            break;
        case INSTR_STACK_DUPN:{
                if(stack.size()==0) throw std::runtime_error("stack undeflow");
                int32_t d=*reinterpret_cast<int32_t*>(&data[pc++]);
                if(d<0||static_cast<uint32_t>(d)>=stack.size()){
                    throw std::runtime_error("invalid stack offset "+std::to_string(d));
                }
                stack.push_back(stack[(stack.size()-1)-d]);
            }
            break;
        case INSTR_STACK_POPN:{
                if(stack.size()==0) throw std::runtime_error("stack undeflow");
                int32_t save=stack.back();
                int32_t d=*reinterpret_cast<int32_t*>(&data[pc++]);
                if(d<0||static_cast<uint32_t>(d+1)>=stack.size()){
                    stack.clear();
                    stack.push_back(save);
                }else{
                    stack.resize(stack.size()-(d+1));
                    stack.push_back(save);
                }
            }
            break;
        case INSTR_STACK_DUP:
            if(stack.size()==0) throw std::runtime_error("stack undeflow");
            stack.push_back(stack.back());
            break;
        case INSTR_STACK_SWP:{
                if(stack.size()<2) throw std::runtime_error("stack undeflow");
                int32_t a=pop();
                int32_t b=pop();
                stack.push_back(a);
                stack.push_back(b);
            }
            break;
        case INSTR_STACK_POP:
            if(stack.size()==0) throw std::runtime_error("stack undeflow");
            stack.pop_back();
            break;
        case INSTR_MATH_ADD:
        case INSTR_MATH_SUB:
        case INSTR_MATH_MUL:
        case INSTR_MATH_DIV:
        case INSTR_MATH_MOD:{
                int32_t rhs=pop();
                int32_t lhs=pop();
                switch(i){
                case INSTR_MATH_ADD:
                    stack.push_back(lhs+rhs);
                    break;
                case INSTR_MATH_SUB:
                    stack.push_back(lhs-rhs);
                    break;
                case INSTR_MATH_MUL:
                    stack.push_back(lhs*rhs);
                    break;
                case INSTR_MATH_DIV:
                    if(rhs==0)throw std::runtime_error("trying to divide by zero");
                    //stack.push_back(lhs/rhs);
                    stack.push_back(floor(static_cast<double>(lhs)/static_cast<double>(rhs)));//shitty hack because of shitty requirements
                    break;
                case INSTR_MATH_MOD:
                    if(rhs==0)throw std::runtime_error("trying to divide by zero");
                    stack.push_back(knuth_mod(lhs,rhs));
                    break;
                default:
                    throw std::runtime_error("unreachable");
                }
            }
            break;
        case INSTR_FC_CSR:
            call_stack.push_back(pc+1);
            pc=data[pc];
            break;
        case INSTR_FC_JMP:
            pc=data[pc];
            break;
        case INSTR_FC_JZ:
            if(pop()==0){
                pc=data[pc];
            }else{
                pc++;
            }
            break;
        case INSTR_FC_JN:
            if(pop()<0){
                pc=data[pc];
            }else{
                pc++;
            }
            break;
        case INSTR_FC_RET:{
                if(call_stack.size()==0) throw std::runtime_error("call stack undeflow");
                pc=call_stack.back();
                call_stack.pop_back();
            }
            break;
        case INSTR_FC_END:
            return true;
        //heap access
        case INSTR_MEM_STORE:{
                int32_t val=pop();
                int32_t addr=pop();
                heap[addr]=val;
            }
            break;
        case INSTR_MEM_LOAD:{
                int32_t addr=pop();
                stack.push_back(heap.at(addr));
            }
            break;
        default:
            throw std::runtime_error("trying to execute invalid instruction");
        }
        return (pc>=data.size());
    }
    instr_t next(){
        if(pc>=data.size())throw std::runtime_error("unexpected end of program");
        uint32_t i=data[pc++];
        if(i>=INSTR_INVALID)throw std::runtime_error("trying to execute invalid instruction");
        return(instr_t)i;
    }
};

struct str_input_provider : public input_provider {
    const std::string &data;
    uint32_t i;

    str_input_provider(const std::string &s):data(s),i(0){
    }

    virtual char next_c() override{
        if(i>=data.size())throw std::runtime_error("EOF");
        return data[i++];
    }

    virtual int next_i() override{
        std::string temp;
        char c;
        while((c=next_c())!='\n'){//while is not eof or newline
            if(c==0)throw std::runtime_error("EOF");
            temp+=c;
        }
        if(temp.size()>2&&temp[0]=='0'&&temp[1]=='x'){//hexadecimal
            return stoi(temp.substr(2),nullptr,16);
        }else{
            return stoi(temp);
        }
    }
};

struct str_output_handler : public output_handler {
    std::string buf;
    virtual void put_c(char c) override {
        buf+=c;
    }
    virtual void put_i(int i) override {
        buf+=std::to_string(i);
    }
};

struct null_input_provider : public input_provider {
    virtual char next_c() override{
        throw std::runtime_error("input not implemented");
    }
    virtual int next_i() override{
        throw std::runtime_error("input not implemented");
    }
};

#include <conio.h>

struct cin_input_provider : public input_provider {
    virtual char next_c() override{
        return getch();
    }
    virtual int next_i() override{
        std::string temp;
    start:
        getline(std::cin,temp);
        if(temp.size()>2&&temp[0]=='0'&&temp[1]=='x'){//hexadecimal
            for(uint32_t i=0;i<temp.size();i++){
                if((temp[i]<'0'||temp[i]>'9')&&(temp[i]<'a'||temp[i]>'z')&&(temp[i]<'A'||temp[i]>'Z'))goto start;
            }
            return stoi(temp.substr(2),nullptr,16);
        }else{
            for(uint32_t i=0;i<temp.size();i++){
                if(temp[i]<'0'||temp[i]>'9')goto start;
            }
            return stoi(temp);
        }
    }
};

struct cout_output_handler : public output_handler {
    virtual void put_c(char c) override {
        std::cout<<c;
    }
    virtual void put_i(int i) override {
        std::cout<<std::to_string(i);
    }
};

//---

#include <cstdio>
#include <stdexcept>

static long find_file_size(FILE * f){
    fseek(f,0,SEEK_END);
    long o=ftell(f);
    fseek(f,0,SEEK_SET);
    return o;
}

std::string readfile(std::string filename){
    std::cout<<"reading file "<<filename<<"\n";
    FILE * f=fopen(filename.c_str(),"r");
    if(!f){
        throw std::runtime_error("could not open file");
    }
    std::string out;
    out.reserve(find_file_size(f));
    for(char c;(c=fgetc(f))!=EOF;)out+=c;
    fclose(f);
    return out;
}



int main(int argc,char ** argv) {
    if(argc!=2){
        printf("usage: whitespace <filename>");
        return 0;
    }
    std::string data(readfile(argv[1]));
    cin_input_provider in;
    cout_output_handler out;
    VM vm(parse(data),in,out);
    while(!vm.step());
    return 0;
}
