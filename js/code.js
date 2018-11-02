// Hacky way for removing blank lines
const code = document.querySelectorAll("pre code");
[...code].forEach(el => el.textContent = el.textContent.replace(/^\n/,''));