// Hacky way for removing pre blank lines
$('pre code').text(function (_, t) {
    return $.trim(t)
});