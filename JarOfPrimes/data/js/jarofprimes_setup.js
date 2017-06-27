var setup = null;
var networks = null;

function validateControls() {
	// console.log("validateControls()");
	
	var result = true;
	var reason = "";
	var ssid = $("#txtSSID").val();
	var passphrase = $("#txtPassphrase").val();
	var passphraserepeat = $("#txtPassphraseRepeat").val();
	
	if (result && ssid) {
		// ssid given. If a passphrase is given as well, it and the 
		// repeat have to match.
		result = ((!passphrase && !passphraserepeat) || (passphraserepeat == passphrase));
		if (!result && !reason) {
			reason = "Passphrases do not match";
		}
	} 
	
	if (result && networks && ssid && !passphrase) {
		// ssid but no passphrase given. Do we know this ssid from a network
		// scan? If so does it report as encrypted?
		result = networks.reduce(function(acc, val) {
			// is this the network-ssid currently entered?
			if (val.ssid == ssid) {
				// Yes. Set result to false if this network reports as enctypted.
				if (val.encrypted) {
					return false;
				}
			};
			// return current result.
			return acc;
		}, result);
		
		if (!result && !reason) {
			reason = "Passphrase required";
		}
	} 
	
	// If a new currentnumber is given, we require confirmation.
	var syncNumber = $("#txtSync").val();
	var syncConfirm = $("input[type=checkbox][name=cbSyncConfirm]:checked").val();

	if (result && syncNumber) {
		result = syncConfirm;
		if (!result && !reason) {
			reason = "Sync requested, but not confirmed";
		}
	}
	
	if (result && syncConfirm) {
		result = syncNumber;
		if (!result && !reason) {
			reason = "Sync confirmed, but no number given";
		}
	}
	
	if (reason) {
		$("#lblReason").text(reason);
		$("#lblReason").fadeIn("slow");
	} else {
		$("#lblReason").fadeOut("slow", function() { $("#lblReason").text(" ") });
	}
	
	return result;
}

function enableControls() {
	// console.log("enableControls()");
	
	var canSave = (setup && validateControls());
	
	if (canSave) {
		$("#btnOk").removeClass("disabled");
	} else {
		$("#btnOk").addClass("disabled");
	}
	
	// Highlight active SSID
	$("a.network").removeClass("active");
	var ssid = $("#txtSSID").val();
	$("a.network[data-ssid=\"" + ssid +"\"]").addClass("active");
	
}

function scan() {
	// console.log("scan()");
	
	networks = null;
	
	return new Promise(function(resolve, reject) {
		// Stagger fade out all items of the list 
		$("#btnScan").addClass("disabled");
		
		var items = $("#divNetworks a.network");
		if (items && items.length) {
			i = items.length-1;
			
			(function fadeOutNetwork(){
				if (i >= 0) {
					$(items[i--]).fadeOut(function() { 
						fadeOutNetwork("fast");
					});
				} else {
					items.remove();
					resolve();
				}
			})();
		} else {
			resolve();
		}
	}).then(function() {
		return new Promise(function(resolve, reject) {
		
			// Show "scanning" item.
			$("#scanScanning").fadeIn();
			$("#scanError").hide();
			$("#scanNotFound").hide();
		
			$.ajax({
				url: "/scan", // ?buster=" + new Date().getTime(),
				dataType: "json"
			}).done(function(data) {
				networks = data;
				
				$("#btnScan").removeClass("disabled");
				$("#scanScanning").hide();
				
				if (!networks.length) {
					$("#scanNotFound").fadeIn();
				} else {
					// Add a new item for each network found but keep them hidden.
					for (var i = 0 ; i < networks.length ; i++) {
						var item = $('<a href="#" class="list-group-item network" style="display: none;" data-ssid="' + networks[i].ssid + '">' + networks[i].ssid + '<span class="' + (networks[i].encrypted ? 'glyphicon glyphicon-lock pull-right' : '') + '"></a>');
						$("#divNetworks").append(item);
					};
					
					// Stagger fade-in of all the networks.
					var items = $("#divNetworks a.network");
					items.click(function(ev) {
						$("#txtSSID").val($(ev.target).attr("data-ssid"));
						enableControls();
					});
					
					i = 0;
					
					(function fadeInNetwork(){
						if (i <= items.length) {
							$(items[i++]).fadeIn("fast", fadeInNetwork)
						}
					})();
				}
				
				enableControls();
				
				resolve();
			}).fail(function(xhr, textStatus, errorThrown) {
				$("#btnScan").removeClass("disabled");
				$("#scanScanning").hide();
				$("#scanError").fadeIn();
				
				enableControls();
				
				reject(errorThrown);
			})
		});
	});
}

function readSetup() {
	// console.log("readSetup()");
	
	setup = null;
	
	return new Promise(function(resolve, reject) {
		$.ajax({
			url: "/setup", // ?buster=" + new Date().getTime(),
			dataType: "json"
		}).done(function(data) {
			setup = data;
			
			// Update controls with the values from setup.
			$("#txtSSID").val(setup.ssid);
			
			enableControls();
			
			resolve();
		}).fail(function(xhr, textStatus, errorThrown) {
			reject(errorThrown);
		})
	});
}

function writeSetup() {
	// console.log("writeSetup()");
	
	if (validateControls) {
		var ssid = $("#txtSSID").val();
		var passphrase = $("#txtPassphrase").val();
		var passphraserepeat = $("#txtPassphraseRepeat").val();
		var syncNumber = $("#txtSync").val();
		var syncConfirm = $("input[type=checkbox][name=cbSyncConfirm]:checked").val();

		setup = {};
		if (ssid) { 
			setup["ssid"] = ssid; 
		};
		if ((passphrase && passphraserepeat && (passphrase == passphraserepeat))) { 
			setup["passphrase"] = passphrase; 
		};
		if (syncNumber && syncConfirm) {
			setup["currentnumber"] = syncNumber;
		};
		
		return new Promise(function(resolve, reject) {
			$.ajax({
				url: "/setup", // ?buster=" + new Date().getTime(),
				method: "POST",
				data: JSON.stringify(setup),
				processData: false,
				contentType: "application/json",
				dataType: "json"
			}).done(function(data) {
				resolve();
			}).fail(function(xhr, textStatus, errorThrown) {
				reject(errorThrown);
			})
		});
	}
}

// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	// console.log("ready()");

	$("input").on("click keyup keypress blur change", function(e) {
		enableControls();
	});

	$("#btnLicense").click(function(ev) {
		location.href = "/license.html";
	});
	
	$("#btnScan").click(function(ev) {
		scan();
	});

	$("#btnCancel").click(function(ev) {
		location.href = "/";
	});
	
	$("#btnOk").click(function(ev) {
		writeSetup().then(function() { location.href = "/"; });
	});
	
	readSetup().then(function() { 
		scan(); 
	});
});
