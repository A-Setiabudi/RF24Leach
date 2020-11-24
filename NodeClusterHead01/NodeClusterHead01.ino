#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(9,10);                    // nRF24L01(+) radio attached using Getting Started board 

RF24Network network(radio);          // Network uses that radio

/*const*/ uint16_t this_node_id;// = 1;
/*const*/ uint16_t this_node;// = 01;        // Address of our node in Octal format
const uint16_t cluster_head_node = 01; // Address of the cluster head node in Octal format
const uint16_t base_station_node = 00;        // Address of the other node in Octal format

bool is_cluster_head;             // Am i a cluster head?

const unsigned long interval = 2000; //ms  // How often to send 'hello world to the other unit

unsigned long last_sent;             // When did we last send?
unsigned long packets_sent;          // How many have we sent already


struct payload_t {                  // Structure of our payload
  uint16_t command;
  unsigned long node_id;
  unsigned long data;
};

void setup(void)
{
  int pilih = 0;
//  int pilih = 1;
//  int pilih = 2;
//  int pilih = 3;
//  int pilih = 4;
   switch(pilih){
    case 0:
      this_node_id = 5;
      this_node = 01;        // Address of our node in Octal format
      break;
    case 1:
      this_node_id = 46;
      this_node = 011;        // Address of our node in Octal format
      break;
    case 2:
      this_node_id = 93;
      this_node = 021;        // Address of our node in Octal format
      break;
    case 3:
      this_node_id = 125;
      this_node = 031;        // Address of our node in Octal format
      break;
    case 4:
      this_node_id = 312;
      this_node = 041;        // Address of our node in Octal format
      break;
  }

  //LED STATUS
  pinMode(8, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("RF24Network/examples/helloworld_tx/");

  if ( this_node == 1 || this_node == 2 ) {
    is_cluster_head = true;
    Serial.println("This is Cluster Head");
  }
  else {
    is_cluster_head = false;
  }
 
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
}

void loop() {
  
  network.update();                          // Check the network regularly

  //========== Receiving ==========//
  while ( network.available() ) {
    RF24NetworkHeader received_header;
    payload_t received_payload;
    network.read(received_header, &received_payload, sizeof(received_payload));
    //Cluster head code
    if (is_cluster_head && packets_sent > 5 ) {
      if ( received_payload.command == 100 ) {
        Serial.println("receiving command to switch cluster head");
        //sent reset command to other node
        payload_t payload = { 150, this_node_id, packets_sent };
        RF24NetworkHeader header(/*to node*/ received_header.from_node);
        bool ok = network.multicast(header,&payload,sizeof(payload),2);  //multicast
        if (ok) {
          Serial.println("Sending ack to CH's candidate...");
          //sent ack to cluster head candidate
          payload_t cluster_head_payload = { 200, this_node_id, packets_sent };
          RF24NetworkHeader cluster_head_header(/*to node*/ received_header.from_node);
          ok = network.write(cluster_head_header,&cluster_head_payload,sizeof(cluster_head_payload));
          packets_sent = 0;
          if (ok) {
            Serial.println("ok. this node is no longer a cluster head");
            is_cluster_head = false;
            network.begin(/*channel*/ 90, /*node address*/ received_header.from_node);
          }
          else {
            Serial.println("failed to send ack to CH's candidate... this node is still a cluster head");
          }
        }
        else {
          Serial.println("failed.");
        }
      }
    }

    //Node listening to others
    else {
      if (received_payload.command == 150) {
        Serial.println("ok. reset.");
        packets_sent = 0;
      }
      else if ( received_payload.command == 200 ) {
        Serial.println("ok. now this node is a cluster head.");
        packets_sent = 0;
        is_cluster_head = true;
        network.begin(/*channel*/ 90, /*node address*/ cluster_head_node);
      }
    }
  }
  
  //========== Sending  ==========//
  unsigned long now = millis();              // If it's time to send a message, send it!
  if ( now - last_sent >= interval ) {
    last_sent = now;

    Serial.print("Sending...");
    payload_t payload = { 0, this_node_id, packets_sent };
    RF24NetworkHeader header(/*to node*/ base_station_node);
    bool ok = network.write(header,&payload,sizeof(payload));
    if (ok) {
      packets_sent++;
      Serial.println("ok.");
    }
    else {
      Serial.println("failed.");
    }

    //Node ask to be a cluster head if already sent 10 or more packets
    if (packets_sent > 10 && is_cluster_head == false) {
      Serial.println("ask for cluster head role");
      payload_t payload = {/*command for asking to be a cluster head*/ 100, this_node_id, packets_sent };
      RF24NetworkHeader header(/*to node*/ cluster_head_node);
      ok = network.write(header,&payload,sizeof(payload));
    }
  }

  //========== LED CH STATUS ==========//
  if (is_cluster_head) {
    digitalWrite(8, HIGH);
  }
  else {
    digitalWrite(8, LOW);
  }
  
}
