/* 
	This is a small script to exercise the Jar of Primes 
	in an unattended fashion to check stability.
*/

// How often to repeat the test?
var TEST_INTERVAL = 20000;

// Replace console.log with something that also logs to the DOM.
function newconsolelog(msg) {
	oldconsolelog(msg);	
	$("#log").append("<span class=\"T\">" + new Date().toString() + ":</span>&nbsp;<span class=\"M\">" + msg + "</span><br/>" );
	
	$("#log :last-child")[0].scrollIntoView();
}

var oldconsolelog = window.console.log;
window.console.log = newconsolelog;

var websocket = null;
var pingInterval = null;

// Returns whether we have an active connected websocket at this moment.
function websocketActive() {
	var result = (websocket && (websocket.readyState === websocket.OPEN));
	return result;
}
		
// Ask Jar of Primes to step to the next number. Remember that the jar will
// continue stepping at its own pace until it hits a Prime number.
function commandStep() {
	if (websocketActive()) {
		// Use WebSocket to command the step.
		console.log("commandStep() - Websocket")
		return new Promise(function(resolve, reject) {
			websocket.send("step");
			resolve();
		})
	} else {
		// Use the /step endpoint to command a step.
		console.log("commandStep() - API")
		return new Promise(function(resolve, reject) {
			$.ajax({
				url: "/step?buster=" + new Date().getTime(),
				dataType: "json"
			}).done(function(data) {
				resolve();
			}).fail(function(xhr, textStatus, errorThrown) {				
				reject(errorThrown);
			})
		});
	}
}

function ping() {
	if (websocketActive()) {
		websocket.send("ping");
	}
}
			
// Connect and setup websocket.
function setupWebSocket() {
	console.log("setupWebSocket()");
	
	var url = "ws://" + location.hostname + ":81/";
	websocket = new WebSocket(url, ["arduino"]);
	
	websocket.onopen = function(ev) {  
		console.log("setupWebSocket() - Websocket opened");
		ping();
		pingInterval = setInterval(ping, 10000);
	};
	
	websocket.onclose = function(ev) {  
		console.log("setupWebSocket() - WebSocket closed code=" + ev.code + ", reason=" + ev.reason + ", wasClean=" + (ev.wasClean ? "true" : "false"));
		clearInterval(pingInterval);
		pingInterval = null;
		
		websocket = null; 
		// Attempt to auto-reconnect.
		setTimeout(setupWebSocket, 10000);
	};
	
	websocket.onerror = function(ev) {    
		console.log("setupWebSocket() - WebSocket Error " + ev.data);
	};
	
	websocket.onmessage = function(ev) {  
		console.log("setupWebSocket() - WebSocket Message " + ev.data);
	};
}
	
function runtest() {
	console.log("runtest() - start");
	commandStep();
	console.log("runtest() - done");
}
	
// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	console.log("ready()");
	
	setupWebSocket();
	
	setInterval(runtest, TEST_INTERVAL);
});