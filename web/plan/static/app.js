

var tempLabelNameToElement = {};
var doorIdToPlan = {};
var config = {};

window.onload = function()
{
  getConfig();
};

function getConfig()
{
  displayStatus.innerHTML = 'Getting config';
  displayStatus.className = 'statusSetup';

  var requestURL = 'http://' + location.host + '/static/config.json';
  var request = new XMLHttpRequest();
  request.open('GET', requestURL);
  request.responseType = 'json';

  request.onload = function()
  {
    if (this.status === 200)
    {
      config = request.response;
      init();
    } else
    {
      displayStatus.innerHTML = 'Failed to get config';
      displayStatus.className = 'statusError';
    }
  };

  request.send();
}

function failedToGetConfig()
{
  displayStatus.innerHTML = 'Failed to get config';
  displayStatus.className = 'statusError';
}

function init()
{
  var displayStatus = document.getElementById('displayStatus');

  // Find which door is on which plan
  findDoors();

  // Hide all status icons for all doors
  for (i = 1; i <= config.DoorCount; i++)
  {
    setDoorState(i, "NOTHING");
    setBellState(i, "OFF");
  }

  // Set all temperature labels to an empty string and populate tempLabelNameToElement
  for (i = 0; i < config.Plans.length; i++)
    initTemperatureLabels(config.Plans[i].Name);


  // Create a new WebSocket.
  displayStatus.innerHTML = 'Connecting';
  displayStatus.className = 'statusSetup';
  var socket = new WebSocket('ws://' + location.host + '/ws');

  // Show a connected message when the WebSocket is opened.
  socket.onopen = function(event)
  {
    displayStatus.innerHTML = 'Connected';
    displayStatus.className = 'statusOpen';
  };

  // Handle any errors that occur.
  socket.onerror = function(error)
  {
    displayStatus.innerHTML = 'Error: ' + error;
    displayStatus.className = 'statusError';
  };


  // Handle events sent by the server.
  socket.onmessage = function(event)
  {
    var message = event.data;
    eventMsg = JSON.parse(message);

    switch (eventMsg.EventType)
    {
      case "DoorState":
        setDoorState(eventMsg.DoorId, eventMsg.Value);
        break;

      case "DoorButton":
        flashBell(eventMsg.DoorId, 50);
        break;

      case "Temperature":
        setTemp(eventMsg.Location, eventMsg.Temperature);
        break;
    }

  };

  // Show a disconnected message when the WebSocket is closed.
  socket.onclose = function(event)
  {
    displayStatus.innerHTML = 'Disconnected!';
    displayStatus.className = 'statusError';
  };

}

function setDoorState(doorId, newState)
{

  switch (newState)
  {
    case "NOTHING":
      initDoorState(doorId);
      break;

    case "OPEN":
      initDoorState(doorId);
      showElement("DOOR_" + doorId + "_OPEN", plan);
      break;

    case "CLOSED":
      initDoorState(doorId);
      showElement("DOOR_" + doorId + "_UNLOCKED", plan);
      break;

    case "LOCKED":
      initDoorState(doorId);
      showElement("DOOR_" + doorId + "_LOCKED", plan);
      break;

    case "WARNING":
    case "FAULT":
    case "UNKNOWN":
      initDoorState(doorId);
      showElement("DOOR_" + doorId + "_WARNING", plan);
      break;
  }
}

function initDoorState(doorId)
{
  plan = doorIdToPlan[doorId];

  hideElement("DOOR_" + doorId + "_OPEN", plan);
  hideElement("DOOR_" + doorId + "_UNLOCKED", plan);
  hideElement("DOOR_" + doorId + "_LOCKED", plan);
  hideElement("DOOR_" + doorId + "_WARNING", plan);
}


function flashBell(doorId, duration)
{
  var countDown = {duration: duration, intervalId: 0, doorId: doorId, visible: false};

  countDown.intervalId = setInterval( function() { bell(countDown); }, 100 );
}

function bell(countDown)
{
  if (--countDown.duration <= 0)
  {
    clearInterval(countDown.intervalId);
    setBellState(countDown.doorId, "OFF");
    return;
  }

  if (countDown.visible == true)
  {
    setBellState(countDown.doorId, "OFF");
    countDown.visible = false;
  } else
  {
    setBellState(countDown.doorId, "ON");
    countDown.visible = true;
  }
}

function setBellState(doorId, state)
{
  plan = doorIdToPlan[doorId];
  element = "DOOR_" + doorId + "_BELL";

  if (state == "ON")
    showElement(element, plan);
  else if (state == "OFF")
    hideElement(element, plan);
}

function hideElement(elementId, plan)
{
  var a = document.getElementById(plan);
  var svgDoc = a.contentDocument;
  var ele = svgDoc.getElementById(elementId);
  if (ele != null)
    ele.style.display = "none";
}

function showElement(elementId, plan)
{
  var a = document.getElementById(plan);
  var svgDoc = a.contentDocument;
  var ele = svgDoc.getElementById(elementId);
  if (ele != null)
    ele.style.display = "block";
}

// Set the temperature label for a given location. Assumes location is unique across all plans
function setTemp(location, value)
{
  tempLabelNameToElement["TEMP_" + location].textContent = value.toFixed(1) + "\u00B0C";
}


// Find all text elements with an ID starting with TEMP_, and set the text to ""
// (hide all the temperature labels until we receive a temperature)
// Also, store a reference to each element/label found in tempLabelNameToElement, so
// later code doesn't need to figure out which plan each label is on.
function initTemperatureLabels(plan)
{
  var a = document.getElementById(plan);
  var svgDoc = a.contentDocument;
  var elements = svgDoc.getElementsByTagName("text");

  for(var i = 0; i < elements.length; i++)
  {
    var labelId = elements[i].getAttribute("id");
    if (labelId.startsWith("TEMP_"))
    {
      tempLabelNameToElement[labelId] = elements[i];
      elements[i].textContent = "";
    }
  }
}

// Find which plan each door is on, and store the mapping in doorIdToPlan
function findDoors()
{
  for (i = 1; i <= config.DoorCount; i++)
  {
    for (j = 0; j < config.Plans.length; j++)
    {
      var a = document.getElementById(config.Plans[j].Name);
      var svgDoc = a.contentDocument;
      var element = svgDoc.getElementById("DOOR_" + i + "_OPEN");
      if (element != null)
        doorIdToPlan[i] = config.Plans[j].Name;
    }
  }
}
