// *********************************************************
// Node.js-powered mockup of the Jar of Primes webapp
// *********************************************************

var http_port = 80;
var currentnumber = 11;

var package_json = require("./package.json");
var morgan = require("morgan");
var express = require("express");
var body_parser = require("body-parser");
var serve_static = require("serve-static")                                           
var http = require("http");
var path = require("path");
var WebSocket = require('ws');

var app = express();

var oneYear = 31557600000;
app.use(serve_static(path.join(__dirname, "../JarOfPrimes/data"), { maxAge: oneYear }));

app.use(body_parser.json());
app.use(body_parser.urlencoded({ extended: true }));

app.use(morgan("dev"));

app.get("/", function(req, res) {
    res.redirect("index.html");
});

function isPrime(candidate) {
	if (candidate == 0) {
		return false;
	} else if (candidate == 1) {
		return false;
	} else if (candidate == 2) {
		return true;
	} else if (candidate == 8648640) {
		return true;
	}  else if ((candidate % 2) == 0) {
		return false;
	} else {  
		testto = Math.floor(Math.sqrt(candidate));
		for (divisor = 3; divisor <= testto; divisor = divisor + 2) {
			if ((candidate % divisor) == 0) {
				return false;
			}
		}
		return true;
	}
}

/*
function test_isPrime(candidate, expected) {
	result = isPrime(candidate);
	console.log("isPrime(" + candidate + ")=" + (result ? "true" : "false") + " expected " + (expected ? "true" : "false") + " " +  ((result == expected) ? "PASS" : "FAIL") + ".");
}

test_isPrime(      0, false);
test_isPrime(      1, false);
test_isPrime(      2, true);
test_isPrime(      3, true);
test_isPrime(      4, false);
test_isPrime(9999990, false);
test_isPrime(9999991, true);
test_isPrime(9999992, false);
*/

var server = null;
var websocketserver = null;

function step() {
	currentnumber = currentnumber + 1;
	if (currentnumber == 1000000) {
		currentnumber = 0;
	}
	
	if (!isPrime(currentnumber)) {
		console.log("[" + currentnumber + "]");
		setImmediate(step);
	} else {
		console.log("[" + currentnumber + "] is prime");
		
		if (websocketserver) {
		websocketserver.clients.forEach(function(client) {
			if (client.readyState === WebSocket.OPEN) {
				client.send(currentnumber);
			}
		});
	}
	}
}

app.use(function (req, res, next) {
	
    var filename = path.basename(req.url);
    var extension = path.extname(filename);
    if (extension === '.css')
        console.log("The file " + filename + " was requested.");
    next();
});

app.get("/step", function(req, res) {
	step();
	res.send({});
});

app.post("/step", function(req, res) {
	step();
	res.send({});
});

app.get("/current", function(req, res) {
	var result = {
		"currentnumber": currentnumber,
		"ssid": "SomeSSID",
		"wifimode": "WIFI_STATION"		
	}
	res.send(result);
});

app.get("/scan", function(req, res) {
	// Simulate a delay in scanning.
	setTimeout(function() {
		//var result = [];
		var result = [
			{ "ssid": "mba-bonn", "encrypted": true },
			{ "ssid": "free for all", "encrypted": false },
			{ "ssid": "Tabaluga", "encrypted": true }					
		];
		res.send(result);
	}, 2000);
});

app.get("/setup", function(req, res) {
	var result = {
		"currentnumber": currentnumber,
		"ssid": "SomeSSID",
		"wifimode": "WIFI_STA"					
	}
	res.send(result);
});

app.post("/setup", function(req, res) {
	console.log(JSON.stringify(req.body, null, 4));
	res.send({});
});

function websocketHeartbeat() {
	this.isAlive = true;
}

server = http.createServer(app).listen(http_port, function () {
	console.log("");
	console.log("Jar of Primes http server listening on port " + http_port + ".");
	
	// Include to test with WebSocket server active, comment out
	// to deactive WebSocket server.
	websocketserver = new WebSocket.Server({ port: http_port+1 });
	console.log("Jar of Primes websocket server listening on port " + (http_port+1) + ".");
	
	websocketserver.on("connection", function(ws, req) {	
		const ip = req.connection.remoteAddress;
		console.log("Websocket-Client connected from " + ip);
		
		ws.on('message', function incoming(message) {
			console.log("Websocket received: " + message);
			switch (message) {
				case "step": step();
			}
		});
	
		ws.on("close", function() {
			console.log('Websocket-Client disconnected');
		});
		
		ws.isAlive = true;
		ws.on("pong", websocketHeartbeat);
	});
	
	var interval = setInterval(function() {
		websocketserver.clients.forEach(function each(ws) {
			if (ws.isAlive === false) {
				return ws.terminate();
			}
			ws.isAlive = false;
			ws.ping("", false, true);
		});
	}, 30000);
	
});
