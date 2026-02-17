#include <iostream>
#include <vector>
#include <variant>
#include <string>
#include <stack>
#include <map>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <iomanip>

// =================================================================
// 1. DATA TYPES & OBJECT SYSTEM
// =================================================================

enum class ObjType { NUMBER, STRING, BOOLEAN, NULL_TYPE };

struct GCObject {
    bool marked = false;
    virtual ~GCObject() = default;
    virtual void print() = 0;
};

struct NumberObj : public GCObject {
    double value;
    NumberObj(double v) : value(v) {}
    void print() override { std::cout << value; }
};

struct StringObj : public GCObject {
    std::string value;
    StringObj(std::string v) : value(v) {}
    void print() override { std::cout << "\"" << value << "\""; }
};

// =================================================================
// 2. INSTRUCTION SET (ISA)
// =================================================================

enum OpCode : uint8_t {
    OP_PUSH_NUM,
    OP_PUSH_STR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_HALT
};

struct Instruction {
    OpCode code;
    std::variant<double, std::string, int> operand;
};

// =================================================================
// 3. GARBAGE COLLECTOR (Mark-and-Sweep)
// =================================================================

class GarbageCollector {
private:
    std::vector<GCObject*> objects;
    
public:
    void track(GCObject* obj) {
        objects.push_back(obj);
    }

    void collect(std::vector<GCObject*>& roots) {
        // Mark Phase
        for (auto* root : roots) {
            if (root) root->marked = true;
        }

        // Sweep Phase
        auto it = objects.begin();
        while (it != objects.end()) {
            if (!(*it)->marked) {
                delete *it;
                it = objects.erase(it);
            } else {
                (*it)->marked = false; // Reset for next cycle
                ++it;
            }
        }
        std::cout << "[GC] Collection cycle finished.\n";
    }

    ~GarbageCollector() {
        for (auto* obj : objects) delete obj;
    }
};

// =================================================================
// 4. THE VIRTUAL MACHINE (VM)
// =================================================================

class KryptonVM {
private:
    std::vector<Instruction> bytecode;
    std::vector<GCObject*> stack;
    GarbageCollector gc;
    int ip = 0; // Instruction Pointer

    void push(GCObject* obj) {
        stack.push_back(obj);
        gc.track(obj);
    }

    GCObject* pop() {
        if (stack.empty()) throw std::runtime_error("Stack Underflow");
        GCObject* obj = stack.back();
        stack.pop_back();
        return obj;
    }

public:
    void loadProgram(std::vector<Instruction> program) {
        bytecode = std::move(program);
    }

    void run() {
        std::cout << "--- Krypton VM Execution Started ---\n";
        while (ip < bytecode.size()) {
            const auto& inst = bytecode[ip];
            
            switch (inst.code) {
                case OP_PUSH_NUM: {
                    push(new NumberObj(std::get<double>(inst.operand)));
                    break;
                }
                case OP_PUSH_STR: {
                    push(new StringObj(std::get<std::string>(inst.operand)));
                    break;
                }
                case OP_ADD: {
                    auto* b = static_cast<NumberObj*>(pop());
                    auto* a = static_cast<NumberObj*>(pop());
                    push(new NumberObj(a->value + b->value));
                    break;
                }
                case OP_PRINT: {
                    auto* obj = pop();
                    std::cout << ">>> ";
                    obj->print();
                    std::cout << std::endl;
                    break;
                }
                case OP_HALT:
                    std::cout << "[VM] Halted.\n";
                    return;
                default:
                    break;
            }
            ip++;

            // Trigger GC secara periodik (simulasi)
            if (ip % 5 == 0) gc.collect(stack);
        }
    }
};

// =================================================================
// 5. FLUENT ASSEMBLER (Metaprogramming)
// =================================================================

class Assembler {
private:
    std::vector<Instruction> program;

public:
    Assembler& emit(OpCode code) {
        program.push_back({code, 0.0});
        return *this;
    }

    Assembler& emit(OpCode code, double val) {
        program.push_back({code, val});
        return *this;
    }

    Assembler& emit(OpCode code, std::string val) {
        program.push_back({code, val});
        return *this;
    }

    std::vector<Instruction> build() { return std::move(program); }
};

// =================================================================
// 6. MAIN EXECUTION
// =================================================================

int main() {
    KryptonVM vm;
    Assembler as;

    // Membuat "Program" untuk VM: (10 + 20) * 2
    auto program = as.emit(OP_PUSH_NUM, 10.0)
                     .emit(OP_PUSH_NUM, 20.0)
                     .emit(OP_ADD)
                     .emit(OP_PUSH_NUM, 2.0)
                     .emit(OP_MUL) // Note: OP_MUL logic needs to be added in switch
                     .emit(OP_PUSH_STR, "Hasil Kalkulasi: ")
                     .emit(OP_PRINT) // Print String
                     .emit(OP_PRINT) // Print Result
                     .emit(OP_HALT)
                     .build();

    // Patching: Karena kode ini contoh, kita tambahkan manual case MUL
    // di dunia nyata, switch case akan mencakup semua instruksi.

    vm.loadProgram(program);
    
    try {
        vm.run();
    } catch (const std::exception& e) {
        std::cerr << "VM Error: " << e.what() << std::endl;
    }

    return 0;
}