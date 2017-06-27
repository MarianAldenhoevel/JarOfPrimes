/* 
	This is the client-part of Jar of Primes responsible for fetching the
	current number from Jar of Primes, animating the odometer and keeping
	it all in sync.
	
	There are two alternative interfaces to the jar:
	
	Http/Polling: Fetches are from /current, and stepping is commanded by /step.
	
	WebSockets: Communication is via WebSockets both ways.
*/

var currentnumber = 0;

var websocket = null;

var UPDATE_INTERVAL_DELAY = 1000; // 0 to deactivate polling in every case.
var ODOMETER_ANIMATION_SPEED = 150;

var pingInterval = null;

var updateInterval = null;
var audio = new Audio("/media/click.mp3");

// Returns whether we have an active connected websocket at this moment.
function websocketActive() {
	var result = (websocket && (websocket.readyState === websocket.OPEN));
	return result;
}
		
// Ask the jar to step to the next number. Remember that the jar will
// continue stepping at its own pace until it hits a Prime number.
function commandStep() {
	if (websocketActive()) {
		// Use WebSocket to command the step.
		return new Promise(function(resolve, reject) {
			websocket.send("step");
			resolve();
		})
	} else {
		// Use the /step endpoint to command a step.
		return new Promise(function(resolve, reject) {
			$.ajax({
				url: "/step", // ?buster=" + new Date().getTime(),
				method: "POST",
				dataType: "json"
			}).done(function(data) {
				resolve();
			}).fail(function(xhr, textStatus, errorThrown) {
				reject(errorThrown);
			})
		});
	}
}

// Fetch the current number displayed in the jar.
function fetch() {
	return new Promise(function(resolve, reject) {
		$.ajax({
			url: "/current", // ?buster=" + new Date().getTime(),
			dataType: "json"
		}).done(function(data) {
			resolve(data.currentnumber);
		}).fail(function(xhr, textStatus, errorThrown) {
			reject(errorThrown);
		})
	});
}

// Setup the odometer.
function initOdometer($this){
	$this.find('.digit').each(function(){
		var $display = $(this);
		var $digit = $display.find('span');

		$digit.html([0,1,2,3,4,5,6,7,8,9,0].reverse().join('<br/>'))
		$digit.css({ 
			top: '-' + (parseInt($display.height()) * (10 - parseInt($digit.attr('title')))) + 'px'
		});
	});
}

// Locally advance the odometer to the next number.
function animateOdometer($this, callback) {
	animateOdometerDigit($this.find('.digit:last'), callback);
}

// Advance a digit on the odometer, with audio, carry-forward and wrap-around.
function animateOdometerDigit($this, callback) {
	var $counter = $this.closest('.counter');
	var $display = $this;
	var $digit = $display.find('span');

	// If we've reached the end of the counter, tick the previous digit
	if(parseInt($digit.css('top')) == -1 * parseInt($display.height())){
		animateOdometerDigit($display.prevAll('.digit:first'), callback);
	}

	audio.pause();
	audio.currentTime = 0;
	audio.play();

	$digit.animate({
		top: '+=' + $display.height() + 'px'
	}, ODOMETER_ANIMATION_SPEED, function() {
		// If we've reached the end of the counter, loop back to the top
		if(parseInt($digit.css('top')) > -1 * parseInt($display.height())){
			$digit.css({
				top: '-' + (parseInt($display.height()) * 10) + 'px'
			});
		}
		
		if (callback) { 
			callback(); 
		};
	});
}

// Clear any polling interval.
function stopUpdatePolling() {
	// console.log("stopUpdatePolling()");
	
	if (updateInterval) {
		clearInterval(updateInterval);
		updateInterval = null;
	}
}

// If required resume polling. It is only required if we do not
// have an active websocket.
function resumeUpdatePolling() {
	// console.log("resumeUpdatePolling()");
	
	if (!websocketActive() && UPDATE_INTERVAL_DELAY) {
		updateInterval = setInterval(update, UPDATE_INTERVAL_DELAY);
	}
}
			
// Update the odometer to a given number without animation. Also
// update the URL on the prime-tester-button.
function displaynumber(number) {
	// console.log("displaynumber(" + number + ")")
	
	var n = number;
	for(var i = 0; i<7 ; i++) {
		figure = n % 10;
		n = Math.floor(n / 10);
		var title = $("#currentnumber span.10to" + i).attr("title");
		if (title != figure) {
			$("#currentnumber span.10to" + i).attr("title", figure);
		}
	}
	initOdometer($("#currentnumber"));
	
	var url = "https://www.wolframalpha.com/input/?i=factor(" + number + ")";
	$("#btnTest").attr("href", url);
}
	
// Update the odometer to a given number by animation.
function updateTo(number) {
	// console.log("updateTo(" +  number + ")");
	
	steps = number - currentnumber;		
	if (steps != 0) {
		currentnumber = number;
		if (steps < 0) {
			// You've broken math. Just update and be done with.
			displaynumber(currentnumber);
			resumeUpdatePolling();
		} else {
			// Animate each step in turn.
			f = function(count) {
				animateOdometer($("#currentnumber"), function() {
					count = count - 1
					if (count > 0) {
						// More steps to go, re-animate
						f(count);
					} else { 
						// Done. Display once more so we update the tester-URL and 
						// then resume polling until a new update happens.
						if (currentnumber == number) {
							displaynumber(currentnumber);
							resumeUpdatePolling();
						}
					}
				});
			};
			f(steps);
		}
	} else {
		resumeUpdatePolling();
	}
}

// Fetch current number from the API and update to it. This is used
// in polling mode.
function update() {
	// console.log("update()");
	
	stopUpdatePolling();
	
	fetch().then(function(data) {
		updateTo(data);
	})
}

// Like update(), but skipping intermediate animated steps.
function initialize() {
	// console.log("initialize()");
	
	stopUpdatePolling();
	
	fetch().then(function(data) {
		currentnumber = data;
		displaynumber(currentnumber);
		resumeUpdatePolling();
	})
}

function ping() {
	if (websocketActive()) {
		websocket.send("ping");
	}
}

// Connect and setup websocket.
function setupWebSocket() {
	// console.log("setupWebSocket()");
	
	var url = "ws://" + location.hostname + ":81/";
	websocket = new WebSocket(url, ["arduino"]);
	
	websocket.onopen = function(ev) {  
		// console.log("Websocket opened");
		stopUpdatePolling();
		ping();
		pingInterval = setInterval(ping, 10000);
	};
	
	websocket.onclose = function(ev) {  
		// console.log("WebSocket closed code=" + ev.code + ", reason=" + ev.reason + ", wasClean=" + (ev.wasClean ? "true" : "false"));
		clearInterval(pingInterval);
		pingInterval = null;
		
		websocket = null; 
		// Attempt to auto-reconnect.
		setTimeout(setupWebSocket, 10000);
	};
	
	websocket.onerror = function(ev) {    
		// console.log("WebSocket Error " + ev.data);
	};
	
	websocket.onmessage = function(ev) {  
		// console.log("WebSocket Message " + ev.data);
		
		// If the message received is just a number then update our
		// odometer to that.
		if (!isNaN(ev.data) && (ev.data != currentnumber)) {
			stopUpdatePolling();
			updateTo(ev.data);
		}
	};
}
	
// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	// console.log("ready()");
	
	$("#btnNext").click(function(ev) {
		commandStep().then(function() { 
			if (!websocketActive()) {
				update();
			}
		});
	});
	
	setupWebSocket();
	initialize();
});