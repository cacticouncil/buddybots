(function (console) {

    console.save = function (data, filename) {

        if (!data) {
            console.error('Console.save: No data')
            return;
        }

        if (!filename) filename = 'console.py'

        if (typeof data === "object") {
            data = JSON.stringify(data, undefined, 4)
        }

        var blob = new Blob([data], { type: 'text/py' }),
            e = document.createEvent('MouseEvents'),
            a = document.createElement('a')

        a.download = filename
        a.href = window.URL.createObjectURL(blob)
        a.dataset.downloadurl = ['text/json', a.download, a.href].join(':')
        e.initMouseEvent('click', true, false, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null)
        a.dispatchEvent(e)
    }
})(console)

function WriteToFile() {
    var author = document.getElementById("author")
    //if (typeof author === "object") {
    //    author = JSON.stringify(author, undefined, 4)
    //}
    var ui_name = document.getElementById("ui_name")
    //if (typeof ui_name === "object") {
    //    ui_name = JSON.stringify(ui_name, undefined, 4)
    //}
    var filename = ui_name + ".def"

    var blob = new Blob([author], { type: 'text/json' }),
           e = document.createEvent('MouseEvents'),
           a = document.createElement('a')

    a.download = filename
    a.href = window.URL.createObjectURL(blob)
    a.dataset.downloadurl = ['text/json', a.download, a.href].join(':')
    e.initMouseEvent('click', true, false, window, 0, 0, 0, 0, 0, false, false, false, false, 0, null)
    a.dispatchEvent(e)

}