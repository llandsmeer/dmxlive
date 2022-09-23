#pragma once

#include <iostream>
#include <cstdlib>
#include <map>
#include <rtmidi/RtMidi.h>

struct Midi {
    RtMidiIn * midiin = 0;
    std::vector<unsigned char> message;

    Midi() {
        try {
            midiin = new RtMidiIn();
            unsigned int nPorts = midiin->getPortCount();
            int selected_port = 0;
            for ( unsigned i=0; i<nPorts; i++ ) {
                std::string port_name = midiin->getPortName(i);
                std::cout << "    Input Port #" << i << ": " << port_name << '\n';
                if (port_name.find("Through") == std::string::npos) {
                    selected_port = i;
                }
            }
            std::cout << "selecting midi port #" << selected_port << std::endl;
            midiin->openPort(selected_port);
        } catch (RtMidiError & error) {
            midiin = 0;
            error.printMessage();
        }
    }

    typedef enum MessageType {
        NoMessage,
        NoteOn,
        NoteOff,
        Value,
        Unknown
    } MessageType;

    struct Message {
        MessageType type;
        uint8_t channel;
        uint8_t key;
        uint8_t value;
        bool empty() {
            return type == NoMessage;
        }
        std::string variable_name() {
            if (type == Unknown) {
                return std::string("unknown");
            }
            if (type == NoMessage) {
                return std::string("empty");
            }
            if (channel > 0) {
                return
                    std::string("n") +
                        std::to_string((int)channel) +
                    (type == Value ? std::string("v") : std::string("k")) +
                        std::to_string((int)key);
            } else {
                return (type == Value ? std::string("v") : std::string("k")) +
                    std::to_string((int)key);
            }
        };
    };

    Message get_message() {
        Message m;
        midiin->getMessage(&message);
        int nbytes = message.size();
        if (nbytes == 0) {
            m.type = NoMessage;
            return m;
        }
        int status = message[0] >> 4;
        m.channel =  message[0] & 0xf;
        if (status == 0x8 /* Note Off */) {
            m.type = NoteOff;
            m.key = message[1];
            m.value = message[2];
        } else if (status == 0x9 /* Note On */) {
            m.type = NoteOn;
            m.key = message[1];
            m.value = message[2];
        } else if (status == 0xb /* Value */) {
            m.type = Value;
            m.key = message[1];
            m.value = message[2];
        } else {
            m.type = Unknown;
        }
        return m;
    }

    ~Midi() {
        if (midiin) {
            delete midiin;
            midiin = 0;
        }
    }
};
