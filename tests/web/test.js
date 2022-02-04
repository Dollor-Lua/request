const click = document.getElementById("click");
const clickText = document.getElementById("clicks");

var clicks = 0;

click.onclick = function (e) {
    clicks++;
    clickText.innerHTML = `${clicks} Clicks`;
};
