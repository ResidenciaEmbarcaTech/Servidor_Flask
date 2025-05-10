/**
 * Cliente HTTP para Raspberry Pi Pico W
 */
#include <stdio.h>
#include "pico/stdio.h"
#include "pico/cyw43_arch.h"
#include "pico/async_context.h"
#include "lwip/altcp_tls.h"
#include "example_http_client_util.h"
#include "hardware/adc.h"
#include <math.h>

#define HOST "172.20.10.13" 
#define PORT 5000
#define INTERVALO_MS 500
#define BUTTON_A 5
#define CICLOS_TEMP   5   // Envia temp+direcao a cada 5 ciclos (≈2.5s)
#define DEADZONE      0.1f  // Zona morta do joystick


static const float RAD2DEG = 180.0f / M_PI;

// Função auxiliar para enviar um GET ao caminho especificado
static void http_send(const char *path) {
    EXAMPLE_HTTP_REQUEST_T req = { 0 };
    req.hostname   = HOST;
    req.url        = path;
    req.port       = PORT;
    req.headers_fn = http_client_header_print_fn;
    req.recv_fn    = http_client_receive_print_fn;
    int res = http_client_request_sync(cyw43_arch_async_context(), &req);
    if (res != 0) {
        printf("Erro HTTP %d em %s\n", res, path);
    }
}

int main()
{
    // Inicializando entradas padrão
    stdio_init_all();

    adc_init();     // Inicializa o módulo ADC :contentReference[oaicite:0]{index=0}
    adc_set_temp_sensor_enabled(true);         // Liga o sensor interno :contentReference[oaicite:1]{index=1}
    // Configura os pinos GPIO 26 e 27 como entradas de ADC (alta impedância, sem resistores pull-up)
    adc_gpio_init(26);  
    adc_gpio_init(27);
    
    // Configuração do Pino do Botão
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    printf("\nIniciando cliente HTTP...\n");

    if (cyw43_arch_init())
    {
        printf("Erro ao inicializar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Conectando a %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Falha na conexão Wi-Fi\n");
        return 1;
    }

    printf("Conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    // Loop Principal
    int button_state = 1; // Variável para controlar o estado do botão (1 = solto, 0 = pressionado)
    uint32_t contador = 0;

    while (1) {

    // --- Leitura raw do joystick ---
        adc_select_input(1);
        uint16_t raw_x = adc_read();
        adc_select_input(0);
        uint16_t raw_y = adc_read();

        // --- Normaliza para [-1.0, +1.0] ---
        float x = ((int)raw_x - 2048) / 2048.0f;
        float y = ((int)raw_y - 2048) / 2048.0f;

        // --- Envia X e Y ao servidor ---
        char path_xy[64];
        snprintf(path_xy, sizeof(path_xy),
                 "/joystick?x=%.2f&y=%.2f", x, y);
        http_send(path_xy);

        const char *vento = "Centro";
        if (fabsf(x) > DEADZONE || fabsf(y) > DEADZONE) {
            float ang = atan2f(y, x) * RAD2DEG;
            if (ang < 0) ang += 360.0f;
            if (ang < 22.5f || ang >= 337.5f)         vento = "Leste";
            else if (ang < 67.5f)                    vento = "Nordeste";
            else if (ang < 112.5f)                   vento = "Norte";
            else if (ang < 157.5f)                   vento = "Noroeste";
            else if (ang < 202.5f)                   vento = "Oeste";
            else if (ang < 247.5f)                   vento = "Sudoeste";
            else if (ang < 292.5f)                   vento = "Sul";
            else                                      vento = "Sudeste";
        }

        char path_dir[48];
        snprintf(path_dir, sizeof(path_dir),
                 "/direcao?valor=%s", vento);
        http_send(path_dir);


        // A cada 5 ciclos (~5 * INTERVALO_MS ms) envia temperatura
    if (++contador >= 5) {
        contador = 0;

        adc_select_input(4);
        uint16_t raw = adc_read();
        const float conv = 3.3f / (1 << 12);
        float temp = 27.0f - ((raw * conv) - 0.706f) / 0.001721f;

        char temp_path[32];
        snprintf(temp_path, sizeof(temp_path), "/TEMP?value=%.2f", temp);

        EXAMPLE_HTTP_REQUEST_T treq = {
            .hostname   = HOST,
            .url        = temp_path,
            .port       = PORT,
            .headers_fn = http_client_header_print_fn,
            .recv_fn    = http_client_receive_print_fn
        };
        http_client_request_sync(cyw43_arch_async_context(), &treq);
    }


        const char *path = NULL;

        // Se o botão A for apertado
        if (gpio_get(BUTTON_A) == 0)
        {
            if (button_state == 1)
            {                    // Se o botão estava solto e agora foi pressionado
                path = "/CLICK"; // Envia a mensagem "CLICK"
                button_state = 0; // Atualiza o estado para pressionado
            }
        }
        else
        {
            if (button_state == 0)
            {                    // Se o botão estava pressionado e agora foi solto
                path = "/SOLTO"; // Envia a mensagem "SOLTO"
                button_state = 1; // Atualiza o estado para solto
            }
        }

        if (path != NULL)
        {
            EXAMPLE_HTTP_REQUEST_T req = {0};
            req.hostname = HOST;
            req.url = path;
            req.port = PORT;
            req.headers_fn = http_client_header_print_fn;
            req.recv_fn = http_client_receive_print_fn;

            printf("Enviando comando: %s\n", path);
            int result = http_client_request_sync(cyw43_arch_async_context(), &req);

            if (result == 0)
            {
                printf("Comando enviado com sucesso!\n");
            }
            else
            {
                printf("Erro ao enviar comando: %d\n", result);
            }

            sleep_ms(20); // Evita múltiplos envios rápidos
        }

        sleep_ms(INTERVALO_MS);
    }
}
