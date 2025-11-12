#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <string.h>
#include <zephyr/random/random.h>

/* Buffers for MQTT client */
static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];

/* MQTT client context */
static struct mqtt_client client_ctx;

/* MQTT Broker address */
static struct sockaddr_storage broker;

bool connected = false;

/* MQTT Event Handler */
void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
    switch (evt->type) {

        case MQTT_EVT_CONNACK:
            if (evt->result == 0) {
                connected = true;
                printk("[MQTT] Connected to broker!\n");
            } else {
                printk("[MQTT] Connection failed: %d\n", evt->result);
            }
            break;

        case MQTT_EVT_DISCONNECT:
            connected = false;
            printk("[MQTT] Disconnected from broker\n");
            break;

        case MQTT_EVT_PUBLISH:
        {
            struct mqtt_publish_param *p = &evt->param.publish;
            // Print topic
            printk("[MQTT] Received topic: %.*s\n",
                p->message.topic.topic.size,
                p->message.topic.topic.utf8);
            // Safely read payload 
            int bytes_read = mqtt_read_publish_payload(client, rx_buffer, sizeof(rx_buffer));
            if (bytes_read > 0) {
                printk("[MQTT] Message: %.*s\n", bytes_read, rx_buffer);
            } else {
                printk("[MQTT] No payload or read error\n");
            }
            break;
        }

        default:
            break;
    }
}

int main(void)
{
    int rc;
    struct zsock_pollfd fds[1];

    printk("Starting Zephyr MQTT Sensor Simulator on %s\n", CONFIG_BOARD);

    /* Initialize MQTT client */
    mqtt_client_init(&client_ctx);

    /* Setup broker address (localhost example) */
    struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(1883);
    inet_pton(AF_INET, "192.168.1.100", &broker4->sin_addr);

    /* Generate a unique client ID */
    char client_id[32];
    snprintf(client_id, sizeof(client_id), "zephyr_mqtt_client_%u", sys_rand32_get());

    /* Configure MQTT client */
    client_ctx.broker = &broker;
    client_ctx.evt_cb = mqtt_evt_handler;
    client_ctx.client_id.utf8 = (uint8_t *)client_id;
    client_ctx.client_id.size = strlen(client_id);
    client_ctx.password = NULL;
    client_ctx.user_name = NULL;
    client_ctx.protocol_version = MQTT_VERSION_3_1_1;
    client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

    client_ctx.rx_buf = rx_buffer;
    client_ctx.rx_buf_size = sizeof(rx_buffer);
    client_ctx.tx_buf = tx_buffer;
    client_ctx.tx_buf_size = sizeof(tx_buffer);

    /* Connect to broker */
    rc = mqtt_connect(&client_ctx);
    if (rc != 0) {
        printk("[MQTT] mqtt_connect failed: %d\n", rc);
        return rc;
    }

    /* Poll socket for connection response */
    fds[0].fd = client_ctx.transport.tcp.sock;
    fds[0].events = ZSOCK_POLLIN;
    poll(fds, 1, 5000);
    mqtt_input(&client_ctx);

    if (!connected) {
        printk("[MQTT] Connection timeout or failed\n");
        mqtt_abort(&client_ctx);
        return -1;
    }

    // Subscribe to a topic after connection
    struct mqtt_topic sub_topics[1] = {
        { .topic = { .utf8 = (uint8_t *)"rtest", .size = strlen("rtest") },
            .qos = MQTT_QOS_1_AT_LEAST_ONCE }
    };

    struct mqtt_subscription_list sub_list = {
        .list = sub_topics,
        .list_count = 1,
        .message_id = sys_rand32_get()
    };

    int rs = mqtt_subscribe(&client_ctx, &sub_list);
    if (rs != 0) {
        printk("[MQTT] Subscribe failed: %d\n", rs);
    } else {
        printk("[MQTT] Subscribe request sent, message_id=%d\n", sub_list.message_id);
    }
    
    while(1){

        /* Process incoming MQTT messages */
        mqtt_input(&client_ctx);
        mqtt_live(&client_ctx);

        // Generate random integers (0–10000) then map to desired range
        uint32_t rand_temp = sys_rand32_get();
        uint32_t rand_humi = sys_rand32_get();

        // Convert to realistic float ranges
        double temperature = 20.0f + (rand_temp % 1500) / 100.0f;  // 20.0–35.0 °C
        double humidity    = 30.0f + (rand_humi % 7000) / 100.0f;  // 30.0–100.0 %

        // Prepare and publish MQTT message
        int len = snprintf(tx_buffer, sizeof(tx_buffer),
                           "Temperature: %.2f°C, Humidity: %.2f%%", temperature, humidity);

        // Log the sensor data
        printk("[SENSOR] %s\n", tx_buffer);

        struct mqtt_publish_param param = {
            .message.topic.qos = MQTT_QOS_2_EXACTLY_ONCE,
            .message.topic.topic.utf8 = (uint8_t *)"sensors/temperature_humidity",
            .message.topic.topic.size = strlen("sensors/temperature_humidity"),
            .message.payload.data = tx_buffer,
            .message.payload.len = len,
            .message_id = sys_rand32_get(),
            .dup_flag = 0,
            .retain_flag = 0,
        };
        
        // Publish the message
        int res = mqtt_publish(&client_ctx,&param);
        if (res != 0) {
            printk("[MQTT] Publish failed: %d\n", res);
        } else {
            printk("[MQTT] Publish successful\n");
        }
        k_sleep(K_SECONDS(1));
    }

    return 0;

}



