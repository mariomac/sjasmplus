#ifndef SJASMPLUS_CODEEMITTER_H
#define SJASMPLUS_CODEEMITTER_H

#include "memory.h"
#include "asm/common.h"

using boost::optional;

enum class OutputMode {
    Truncate, Rewind, Append
};

class CodeEmitter {

private:

    Assembler &Asm;
    uint16_t CPUAddress;
    uint16_t EmitAddress; // = CPUAddress unless the DISP directive is used
    bool Disp; // DISP flag
    bool CPUAddrOverflow;
    bool EmitAddrOverflow;

    int Slot = -1;

    MemoryManager MemManager;

    fs::path RawOutputFileName;
    bool RawOutputEnable = false;
    bool RawOutputOverride = false;
    fs::fstream RawOFS;
    uintmax_t ForcedRawOutputSize = 0;
    fs::path ForcedOutputDirectory;

    void enforceFileSize();

public:
    CodeEmitter() = delete;
    explicit CodeEmitter(Assembler &_Asm) : Asm(_Asm) {
	reset();
    }

    ~CodeEmitter() {
        if (RawOFS.is_open()) {
            RawOFS.close();
            enforceFileSize();
        }
    }

    uint16_t getCPUAddress() {
        return CPUAddress;
    }

    uint16_t getEmitAddress() {
        return Disp ? EmitAddress : CPUAddress;
    }

    optional<std::string> emitByte(uint8_t Byte);

    // ORG directive
    void setAddress(uint16_t NewAddress) {
        if (!Disp) {
            CPUAddress = NewAddress;
            CPUAddrOverflow = false;
        } else {
            EmitAddress = NewAddress;
            EmitAddrOverflow = false;
        }
    }

    // Increase address and return true on overflow
    bool incAddress();

    // DISP directive
    void doDisp(uint16_t DispAddress);

    // ENT directive (undoes DISP)
    void doEnt();

    bool isDisp() { return Disp; }

    optional<std::string> align(uint16_t Alignment, optional<uint8_t> FillByte);

    void reset() {
        CPUAddress = EmitAddress = 0;
        Disp = CPUAddrOverflow = EmitAddrOverflow = false;
        Slot = isMemManagerActive() ? MemManager.defaultSlot() : -1;
    }

    bool isMemManagerActive() { return MemManager.isActive(); }

    void setMemModel(const std::string &Name) {
        MemManager.setMemModel(Name);
        Slot = MemManager.defaultSlot();
    }

    MemModel &getMemModel() {
        return MemManager.getMemModel();
    }

    const std::string &getMemModelName() {
        return MemManager.getMemModelName();
    }

    bool isPagedMemory() {
        return MemManager.isActive() && MemManager.isPagedMemory();
    }

    int numMemPages() {
        return MemManager.numMemPages();
    }

    int getPageNumInSlot(int Slot) {
        return MemManager.getPageNumInSlot(Slot);
    }

    // Returns an error string in case of failure
    optional<std::string> setPage(int Page) {
        return MemManager.setPage(Slot, Page);
    }

    // Save slot number set by the SLOT directive
    optional<std::string> setSlot(int NewSlot) {
        auto Err = MemManager.validateSlot(NewSlot);
        if (Err) return Err;
        else {
            Slot = NewSlot;
            return boost::none;
        }
    }

    int getPage() {
        return MemManager.getPageForAddress(getEmitAddress());
    }

    uint8_t getByte(uint16_t Addr) {
        uint8_t Byte;
        getBytes(&Byte, Addr, 1);
        return Byte;
    }

    void getBytes(uint8_t *Dest, uint16_t Addr, uint16_t Size) {
        MemManager.getBytes(Dest, Addr, Size);
    }

    void getBytes(uint8_t *Dest, int Slot, uint16_t AddrInPage, uint16_t Size) {
        MemManager.getBytes(Dest, Slot, AddrInPage, Size);
    }

    const uint8_t *getPtrToMem() {
        return MemManager.getPtrToMem();
    }

    const uint8_t *getPtrToPage(int Page) {
        return MemManager.getPtrToPage(Page);
    }

    const uint8_t *getPtrToPageInSlot(int Slot) {
        return MemManager.getPtrToPageInSlot(Slot);
    }

    void setRawOutputOptions(bool EnableOrOverride,
                             const fs::path &FileName,
                             const fs::path &_ForcedOutputDirectory);

    void setRawOutput(const fs::path &FileName, OutputMode Mode = OutputMode::Truncate);

    bool isRawOutputEnabled() { return RawOutputEnable; }

    bool isRawOutputOverriden() { return RawOutputOverride; }

    optional<std::string> seekRawOutput(std::streamoff Offset, std::ios_base::seekdir Method);

    void setForcedRawOutputFileSize(uintmax_t NewSize) { ForcedRawOutputSize = NewSize; }

    bool isForcedRawOutputSize() { return ForcedRawOutputSize > 0; }

    fs::path resolveOutputPath(const fs::path &p);
};

fs::path resolveOutputPath(const fs::path &p);

#endif //SJASMPLUS_CODEEMITTER_H
