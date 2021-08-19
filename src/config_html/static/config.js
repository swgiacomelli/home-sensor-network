function load_networks() {
    fetch("/networks")
        .then(res => res.json())
        .then((networks) => {
            wifiNetworks = document.getElementById("wifiNetworks");
            wifiNetworks.textContent = "";
            networks.forEach(item => {
                let option = document.createElement('option');
                option.value = item;
                wifiNetworks.appendChild(option);
            });
        })
        .catch(err => { throw err });
}

function load_values() {
    fetch("/values")
        .then(res => res.json())
        .then((values) => {
            for (var key of Object.keys(values)) {
                document.getElementById(key).value = values[key];
            }
        })
        .catch(err => { throw err });

}

const resultsModal = document.getElementById("resultsModal");

document.getElementsByClassName("close")[0].onclick = function () {
    resultsModal.style.display = "none";
}

window.onclick = function (event) {
    if (event.target == resultsModal) {
        resultsModal.style.display = "none";
    }
}

function showResultsModal() {
    resultsModal.style.display = "block";
}

window.addEventListener("load", function () {
    function sendData() {
        const fd = new FormData(form);
        var data = new Map();
        fd.forEach((value, key) => {
            data[key] = value;
        });
        fetch("/update",
            {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                }, body: JSON.stringify(data)
            }).then(res => res.json()).then((response) => {
                document.getElementById("resultsOutput").textContent = response.message;
                showResultsModal();
            }).catch((err) => {
                document.getElementById("resultsOutput").textContent = "Unexpected error while configuring device.";
                console.log(err);
                showResultsModal();
            });
    }

    const form = document.getElementById("deviceConfigurationForm");

    form.addEventListener("submit", function (event) {
        event.preventDefault();
        sendData();
    });


});

load_values();
load_networks();