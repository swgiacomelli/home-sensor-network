var deviceID = "test";
var networks = ["net a", "net b"];

document.getElementById("deviceID").setAttribute("value", deviceID);
document.getElementById("wifiNetworks").innerHTML = networks.map(v => "<option>" + v + "</option>").join("");