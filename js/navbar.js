$(document).ready(function() {
    $('#sidenav').load('navbar.html#navbar');
    setTimeout(function(){ 
         $('a.navresume').addClass("active");
         $('a.navwork').addClass("active");
    }, 0);
 });