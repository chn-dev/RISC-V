#include <cstdint>

class RISCV
{
public:
   class MemoryInterface
   {
   public:
      virtual uint8_t readMem8( uint32_t address ) = 0;
      virtual uint16_t readMem16( uint32_t address ) = 0;
      virtual uint32_t readMem32( uint32_t address ) = 0;
      virtual void writeMem8( uint32_t address, uint8_t d ) = 0;
      virtual void writeMem16( uint32_t address, uint16_t d ) = 0;
      virtual void writeMem32( uint32_t address, uint32_t d ) = 0;
   };

   RISCV( MemoryInterface *pMem );
   ~RISCV();

   void step();
   void reset();

   static std::string registerName( int n );

private:
   void unknownOpcode();

   uint32_t getRegister( int r );
   void setRegister( int r, uint32_t v );

   MemoryInterface *m_pMemory;
   uint32_t m_Registers[32];
   uint32_t m_PC;
};

