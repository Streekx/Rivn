#ifndef RIVEN_CODEGEN_HPP
#define RIVEN_CODEGEN_HPP

#include "../lir/lir.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace Riven {

class Codegen {
public:
    enum class Target {
        X86_64,
        ARM64
    };

private:
    Target target;

public:
    Codegen(Target tgt = Target::X86_64) : target(tgt) {}

    std::string generateAssembly(const std::vector<LIRInstruction>& lirInstructions) {
        std::stringstream ss;
        if (target == Target::X86_64) {
            ss << ".intel_syntax noprefix\n";
            ss << ".global _start\n";
            ss << ".text\n";
            ss << "_start:\n";
            ss << "  push rbp\n";
            ss << "  mov rbp, rsp\n";
            ss << "  sub rsp, 32\n\n";

            for (const auto& lir : lirInstructions) {
                // Formatting instructions properly to emulate real x86 instructions
                if (lir.op == LIRInstruction::OpCode::MOV) {
                    ss << "  mov " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::ADD) {
                    ss << "  add " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::SUB) {
                    ss << "  sub " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::IMUL) {
                    ss << "  imul " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::CMP) {
                    ss << "  cmp " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JE) {
                    ss << "  je " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JNE) {
                    ss << "  jne " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JL) {
                    ss << "  jl " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JG) {
                    ss << "  jg " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JMP) {
                    ss << "  jmp " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::PUSH) {
                    ss << "  push " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::POP) {
                    ss << "  pop " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::CALL) {
                    ss << "  call " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::RET) {
                    ss << "  ret\n";
                } else if (lir.op == LIRInstruction::OpCode::SYSCALL) {
                    ss << "  syscall\n";
                }
            }

            ss << "\n";
            ss << "  mov rsp, rbp\n";
            ss << "  pop rbp\n";
            // Sys module exit sequence for AMD64
            ss << "  mov rax, 60\n";
            ss << "  mov rdi, 0\n";
            ss << "  syscall\n";
        } else {
            // ARM64 target
            ss << ".global _start\n";
            ss << ".align 2\n";
            ss << "_start:\n";
            ss << "  stp x29, x30, [sp, #-32]!\n";
            ss << "  mov x29, sp\n\n";

            for (const auto& lir : lirInstructions) {
                if (lir.op == LIRInstruction::OpCode::MOV) {
                    ss << "  mov " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::ADD) {
                    ss << "  add " << lir.regDest << ", " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::SUB) {
                    ss << "  sub " << lir.regDest << ", " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::IMUL) {
                    ss << "  mul " << lir.regDest << ", " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::CMP) {
                    ss << "  cmp " << lir.regDest << ", " << lir.arg1 << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JE) {
                    ss << "  b.eq " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JNE) {
                    ss << "  b.ne " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JL) {
                    ss << "  b.lt " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JG) {
                    ss << "  b.gt " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::JMP) {
                    ss << "  b " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::CALL) {
                    ss << "  bl " << lir.regDest << "\n";
                } else if (lir.op == LIRInstruction::OpCode::RET) {
                    ss << "  ret\n";
                } else {
                    ss << "  // unsupported opcode: " << lir.toString() << "\n";
                }
            }

            ss << "\n";
            // Exit command on ARM64 macOS/Linux
            ss << "  mov x0, #0\n";
            ss << "  mov x8, #93\n"; // exit syscall on linux arm
            ss << "  svc #0\n";
        }
        return ss.str();
    }
};

} // namespace Riven

#endif // RIVEN_CODEGEN_HPP
