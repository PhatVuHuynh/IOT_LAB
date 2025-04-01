print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json

BROKER_ADDRESS = "app.coreiot.io"
PORT = 1883
DHT20_ACCESS_TOKEN = "d3P1weVrXAvkGj2bqkDW"
ACCESS_USERNAME = ""

# #HCMUT
# long = 106.65789107082472
# lat = 10.772175109674038

#H6
long = 106.80633605864662
lat = 10.880018410410052

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")


def recv_message(client, userdata, message):
    print("Received: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if "temperature" in jsonobj['client']:
            print("get temp")
    except:
        pass


def connected(client, usedata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe("v1/devices/me/attributes")
        client.subscribe("v1/devices/me/rpc/request/+")
    else:
        print("Connection is failed")


client = mqttclient.Client("dht20")
client.username_pw_set(DHT20_ACCESS_TOKEN)

# client.on_connect = connected
client.connect(BROKER_ADDRESS, 1883)
client.loop_start()

client.on_subscribe = subscribed
client.on_message = recv_message

temp = 30
humi = 50
light_intesity = 100
counter = 0
while True:
    collect_data = {'temperature': temp, 'humidity': humi,
                    'light':light_intesity,
                    'long': long, 'lat': lat}
    
    temp += 1
    humi += 1
    light_intesity += 1
    # client.publish('v1/devices/me/telemetry', json.dumps(collect_data), 1)
    client.publish('v1/devices/me/attributes/', json.dumps(collect_data)) # gửi data
    client.publish('v1/devices/me/attributes/request/1', json.dumps(collect_data)) # nhận data
    time.sleep(5)