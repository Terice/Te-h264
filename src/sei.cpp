#include "sei.h"
#include "parser.h"

sei::sei(parser *p)
{
    pa = p;
}

sei::~sei()
{
    
}


void sei::decode()
{
    int payloadType = 0;
    int payloadSize = 0;
    unsigned char ch;

    while((ch = pa->read_ch()) == 0xFFU)
    {
        payloadType += 255;
    }
    last_payload_type_byte = ch;
    payloadType += last_payload_type_byte;
    
    while((ch = pa->read_ch()) == 0xFFU)
    {
        payloadSize += 255;
    }
    last_payload_size_byte = ch;
    sei_payload(payloadType, payloadSize);
    // std::cout << ">>sei: [" << payloadType <<"], ["<< payloadSize << "]" << std::endl;
}
void sei::sei_payload(int, int)
{
    
}
