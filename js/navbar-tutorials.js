$(document).ready(function() {
    $('#sidenav').load('navbar-tutorials.html#navbar');
    setTimeout(function(){ 
         $('a.navresume').addClass("active");
         $('a.navwork').addClass("active");
    }, 0);
 });