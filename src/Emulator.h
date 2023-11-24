#include <string>

#include "RISCV.h"

class Emulator : public RISCV::MemoryInterface
{
public:
   static Emulator *create( std::string fileName );
   ~Emulator();

   void step();

   virtual uint8_t readMem8( uint32_t address );
   virtual uint16_t readMem16( uint32_t address );
   virtual uint32_t readMem32( uint32_t address );
   virtual void writeMem8( uint32_t address, uint8_t d );
   virtual void writeMem16( uint32_t address, uint16_t d );
   virtual void writeMem32( uint32_t address, uint32_t d );

private:
   Emulator( uint8_t *pProgramData, size_t programDataSize );

   RISCV *m_pCPU;
   uint8_t *m_pProgramData;
   size_t m_ProgramDataSize;
   uint32_t m_ROMStart;
   uint32_t m_RAMStart;
   uint32_t m_RAMSize;
   uint8_t *m_pRAM;
};
