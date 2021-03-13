#ifndef SEI_H__
#define SEI_H__

class parser;

class sei
{
private:
    parser *pa;
    void sei_payload(int, int);
public:
    int last_payload_type_byte;
    int last_payload_size_byte;
    sei(parser *p);
    ~sei();

    void decode();
};

#endif