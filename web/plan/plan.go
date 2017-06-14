package main

import (
  "fmt"
  "io/ioutil"
  "net/http"
  "sync"
  "strconv"
  "flag"
  "os"
  "github.com/eclipse/paho.mqtt.golang"
  "github.com/grafov/bcast"
  "github.com/zieckey/goini"
  "github.com/gorilla/websocket"
)

type msgDoorEvent struct {
  EventType string
  DoorId    int
  Value     string
}

type msgDoorButtonEvent struct {
  EventType string
  DoorId    int
}

type msgTemperatureEvent struct {
  EventType   string
  Location    string
  Temperature float64
}

// Map & lock to store door states
var doorStates = make(map[int]string)
var doorStatesLock = sync.RWMutex{}

// Map & lock to store temperatures
var temperatures = make(map[string]string)
var temperaturesLock = sync.RWMutex{}


// create broadcast group for events
var eventGroup = bcast.NewGroup()

var webBase string

func main() {
  fmt.Printf("Started.\n")

  // Command line params
  configFilePtr := flag.String("c", "plan.conf", "config file to use")
  webBasePathPtr := flag.String("w", "static", "path to web base with index.html etc.")
  flag.Parse()
  fmt.Println("Using config file:", *configFilePtr)


  // Read config
  filename := *configFilePtr
  webBase   = *webBasePathPtr
  ini, err := goini.LoadInheritedINI(filename)
  if err != nil {
    fmt.Printf("parse config file %v failed : %v\n", filename, err.Error())
    return
  }
  
  // Sanity check webBase - print out a warning if index.html isn't there / can't be read
  if _, err := os.Stat(webBase + "/index.html"); os.IsNotExist(err) {
    fmt.Printf("Warning: unable to open [%v]; check web base directory is set correctly (pass -w <web base> flag)\n", webBase + "/index.html")
    fmt.Printf("         Error = [%v]\n", err.Error())
  }

  // get MQTT server connection details
  mqttHost, _ := ini.SectionGet("mqtt", "host")
  mqttPort, _ := ini.SectionGet("mqtt", "port")
  mqttConStr := "tcp://" + mqttHost + ":" + mqttPort

  // get MQTT status reporting topics
  statusRequestTopic, _   := ini.SectionGet("mqtt", "status_request")
  statusResponsetTopic, _ := ini.SectionGet("mqtt", "status_response")
  statusName          , _ := ini.SectionGet("mqtt", "status_name")

  // get port for webserver to listen on
  listenPort          , _ := ini.SectionGet("plan", "listen_port")

  http.HandleFunc("/ws", wsHandler)
  http.HandleFunc("/", rootHandler)

  http.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.Dir(webBase))))

  // setup mqtt connection
  fmt.Printf("Connecting to MQTT server: %v\n", mqttConStr)
  opts := mqtt.NewClientOptions().AddBroker(mqttConStr).SetClientID("plan")
  c := mqtt.NewClient(opts)
  if token := c.Connect(); token.Wait() && token.Error() != nil {
    panic(token.Error())
  }

  go eventGroup.Broadcast(0) // accept messages and broadcast it to all members

  if token := c.Subscribe("nh/gk/+/DoorState", 0, doorStateHandler); token.Wait() && token.Error() != nil {
    panic(token.Error())
  }

  if token := c.Subscribe("nh/gk/+/DoorButton", 0, doorButtonHandler); token.Wait() && token.Error() != nil {
    panic(token.Error())
  }

  if token := c.Subscribe("nh/temperature/+", 0, temperatureHandler); token.Wait() && token.Error() != nil {
    panic(token.Error())
  }

  if token := c.Subscribe(statusRequestTopic, 0, func (client mqtt.Client, msg mqtt.Message) {statusReqHandler(client, msg, statusResponsetTopic, statusName)}); token.Wait() && token.Error() != nil {
    panic(token.Error())
  }

  // Start web server
  listenAddr := ":" + listenPort
  fmt.Printf("Listen on: [%v], web base: [%v]\n", listenAddr, webBase)
  http.ListenAndServe(listenAddr, nil)

}

// Handle events to the DoorState topic
func doorStateHandler(client mqtt.Client, msg mqtt.Message) {
  msgPayload := string(msg.Payload()[:])
  msgTopic := msg.Topic()
  event := msgDoorEvent{}

  fmt.Printf("doorStateHandler         ")
  fmt.Printf("[%s]  ", msgTopic)
  fmt.Printf("%s\n", msgPayload)

  fmt.Sscanf(msgTopic, "nh/gk/%d/DoorState", &event.DoorId )

  doorStatesLock.Lock()
  doorStates[event.DoorId] = msgPayload
  doorStatesLock.Unlock()
  
  event.EventType = "DoorState"
  event.Value = msgPayload
  eventGroup.Send(event)
}

// Handle events to the DoorButton (aka door bell button) topic
func doorButtonHandler(client mqtt.Client, msg mqtt.Message) {
  msgPayload := string(msg.Payload()[:])
  msgTopic := msg.Topic()
  event := msgDoorButtonEvent{}

  fmt.Printf("doorButtonHandler        ")
  fmt.Printf("[%s] ", msgTopic)
  fmt.Printf("%s\n", msgPayload)

  fmt.Sscanf(msgTopic, "nh/gk/%d/DoorButton", &event.DoorId )
  event.EventType = "DoorButton"

  eventGroup.Send(event)
}

// Handle events to the Temperature topic
func temperatureHandler(client mqtt.Client, msg mqtt.Message) {
  msgPayload := string(msg.Payload()[:])
  msgTopic := msg.Topic()
  event := msgTemperatureEvent{}

  fmt.Printf("temperatureHandler       ")
  fmt.Printf("[%s] ", msgTopic)
  fmt.Printf("%s\n", msgPayload)

  fmt.Sscanf(msgTopic, "nh/temperature/%s", &event.Location)

  temperaturesLock.Lock()
  temperatures[event.Location] = msgPayload
  temperaturesLock.Unlock()

  event.EventType = "Temperature"
  event.Temperature, _ = strconv.ParseFloat(msgPayload, 32)

  eventGroup.Send(event)
}

// Handle events to the status topic - we reply to these via MQTT, rather than pass them on to connected web clients
func statusReqHandler(client mqtt.Client, msg mqtt.Message, responseTopic string, statusName string) {
  msgPayload := string(msg.Payload()[:])

  if msgPayload == "STATUS" {
    client.Publish(responseTopic, 0, false, "Running: " + statusName)
  }
}


func rootHandler(w http.ResponseWriter, r *http.Request) {
  content, err := ioutil.ReadFile(webBase + "/index.html")
  if err != nil {
    fmt.Println("Could not open file.", err)
  }
  fmt.Fprintf(w, "%s", content)
}

func wsHandler(w http.ResponseWriter, r *http.Request) {
  if r.Header.Get("Origin") != "http://"+r.Host {
    http.Error(w, "Origin not allowed", 403)
    return
  }
  conn, err := websocket.Upgrade(w, r, w.Header(), 1024, 1024)
  if err != nil {
    http.Error(w, "Could not open websocket connection", http.StatusBadRequest)
  }

  go wsSendEvents(conn)

}

// Send cached door states to a connecting web client
func sendInitalDoorStates(conn *websocket.Conn) {
  doorStatesLock.RLock()
  defer doorStatesLock.RUnlock()
  for key, value := range doorStates {
    conn.WriteJSON(msgDoorEvent{"DoorState", key, value})
  }
}

// Send cached temperatures to a connecting web client
func sendInitalTemperatures(conn *websocket.Conn) {
  temperaturesLock.RLock()
  defer temperaturesLock.RUnlock()
  for key, value := range temperatures {
    event := msgTemperatureEvent{}
    event.EventType = "Temperature"
    event.Location = key
    event.Temperature, _ = strconv.ParseFloat(value, 32)

    conn.WriteJSON(event)
  }
}

// Main websocket handler - one of these (thread/go routine) per connected client
func wsSendEvents(conn *websocket.Conn) {

  // Client has just connected... send inital door states & temperatures
  sendInitalDoorStates(conn)
  sendInitalTemperatures(conn);

  // Join broadcast group to get door events arriving via mqtt
  events := eventGroup.Join()

  // Send door events to connected client. On error (likley client disconnect), exit goroutine
  for {
    event := events.Recv() // Wait for broadcasted events
    if err := conn.WriteJSON(event); err != nil {
      fmt.Println(err)
      return
    }
  }
  return
}
