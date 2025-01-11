const predefinedUsername = "admin";
const predefinedPassword = "pass123";

function login() {
    
    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;
   
    if (username === predefinedUsername && password === predefinedPassword) {
        alert("Login successful!");
        window.location.href = "home.html"; 
    } else {
        document.getElementById("error-message").innerText = "Invalid username or password!";
    }
}

function revealCredentials() {
    const credentials = document.getElementById("credentials");
    credentials.style.display = "block";

    setTimeout(() => {
        credentials.style.display = "none";
    }, 3000); 

}

function showSection(sectionId) {

    const sections = document.querySelectorAll('section');
    sections.forEach(section => {
        section.classList.remove('active');
    });

    const targetSection = document.getElementById(sectionId);
    if (targetSection) {
        targetSection.classList.add('active');
    }
}

function signOut() {
    window.location.href = "index.html";
}