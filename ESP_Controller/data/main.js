function countLetters() {
    var textbox = document.getElementById("text");
    var charcounter = document.getElementById("charcount");
    charcounter.innerHTML = textbox.value.length.toString();
}