#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <bearssl/bearssl.h>
#include <user_interface.h>
#include <WiFiManager.h>

#define CPU_FREQ_MHZ      160
#define LISTEN_PORT       54123
#define LOCAL_PORT        54124
#define ENTROPY_POOL_SIZE 512
#define MIN_ENTROPY_BYTES 128
#define HASH_SIZE         32
#define SIG_SIZE          64

char taas_ip_str[16] = "0.0.0.0";
bool should_save_config = false;

uint16_t taas_port = 54125;

struct __attribute__((packed, aligned(4))) chaos_packet_t {
    uint8_t seed[HASH_SIZE];
    uint8_t signature[SIG_SIZE];
    uint64_t timestamp;
} wire_packet;

typedef enum { STATE_MINING, STATE_TX_HASH, STATE_WAIT_RX } fsm_state_t;

struct system_ctx {
    fsm_state_t state;
    unsigned long state_timer;
    struct {
        uint32_t buffer[ENTROPY_POOL_SIZE / 4]; 
        size_t count;
    } pool;
    bool product_ready;
} sys;

WiFiUDP udp_client;
WiFiUDP udp_server;
IPAddress taas_ip;

void ICACHE_RAM_ATTR mix_entropy(uint32_t data) {
    if (sys.pool.count < (ENTROPY_POOL_SIZE / 4)) {
        sys.pool.buffer[sys.pool.count++] = data;
    }
}

void handle_incoming_requests() {
    if (udp_server.parsePacket()) {
        if (sys.product_ready) {
            udp_server.beginPacket(udp_server.remoteIP(), udp_server.remotePort());
            udp_server.write((uint8_t*)&wire_packet, sizeof(chaos_packet_t));
            udp_server.endPacket();
        }
        udp_server.flush();
    }
}

void setup() {
    system_update_cpu_freq(CPU_FREQ_MHZ);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    if (LittleFS.begin() && LittleFS.exists("/config.ip")) {
        File f = LittleFS.open("/config.ip", "r");
        if (f) {
            f.readBytes(taas_ip_str, sizeof(taas_ip_str));
            f.close();
        }
    }

    WiFiManager wm;
    wm.setSaveConfigCallback([](){ should_save_config = true; });
    WiFiManagerParameter p_ip("server", "Backend IP", taas_ip_str, 16);
    wm.addParameter(&p_ip);

    if (!wm.autoConnect("Entropy_Setup")) ESP.reset();

    if (should_save_config) {
        File f = LittleFS.open("/config.ip", "w");
        f.print(p_ip.getValue());
        f.close();
    }

    udp_client.begin(LOCAL_PORT);
    udp_server.begin(LISTEN_PORT);
    taas_ip.fromString(p_ip.getValue());
    sys.state = STATE_MINING;
}

void loop() {
    handle_incoming_requests();
    unsigned long now = millis();

    switch (sys.state) {
        case STATE_MINING:
            for(int i=0; i<8; i++) {
                uint32_t r = (system_adc_read() << 16) | system_adc_read();
                mix_entropy(r ^ ESP.getCycleCount());
            }
            if (sys.pool.count >= (MIN_ENTROPY_BYTES / 4)) sys.state = STATE_TX_HASH;
            break;

        case STATE_TX_HASH: {
            br_sha256_context ctx;
            br_sha256_init(&ctx);
            br_sha256_update(&ctx, sys.pool.buffer, sys.pool.count * 4);
            br_sha256_out(&ctx, wire_packet.seed);

            udp_client.beginPacket(taas_ip, taas_port);
            udp_client.write(wire_packet.seed, HASH_SIZE);
            udp_client.endPacket();

            sys.pool.count = 0;
            sys.state_timer = now;
            sys.state = STATE_WAIT_RX;
            break;
        }

        case STATE_WAIT_RX:
            if (udp_client.parsePacket() >= 104) {
                uint8_t rb[128];
                udp_client.read(rb, 128);
                memcpy(&wire_packet.timestamp, rb + 32, 8);
                memcpy(wire_packet.signature, rb + 40, SIG_SIZE);
                sys.product_ready = true;
                sys.state = STATE_MINING;
            } else if (now - sys.state_timer > 1000) {
                sys.state = STATE_MINING;
            }
            break;
    }
}
