const QT_SERVER_URL = "http://localhost:8080";
let serverConnected = false;

Office.onReady((info) => {
  if (info.host === Office.HostType.OneNote) {
    document.getElementById("sideload-msg").style.display = "none";
    document.getElementById("app-body").style.display = "flex";
    
    // Set up event listeners
    document.getElementById("capture-btn").onclick = captureScreenshot;
    document.getElementById("refresh-btn").onclick = refreshScreenshots;
    document.getElementById("export-pdf-btn").onclick = exportToPDF;
    
    // Check server connection on load, then refresh screenshots
    checkServerConnection().then(() => {
      if (serverConnected) {
        refreshScreenshots();
      }
    });
  }
});


async function checkServerConnection() {
  const statusElement = document.getElementById("server-status");
  try {
    const response = await fetch(`${QT_SERVER_URL}/status`);
    const data = await response.json();
    if (data.status === "ok") {
      serverConnected = true;
      statusElement.textContent = "✓ Connected to Qt Server";
      statusElement.className = "ms-font-m connected";
    } 
    else {
      throw new Error("Invalid response");
    }
  } 
  catch (error) {
    serverConnected = false;
    statusElement.textContent = "✗ Qt Server not running (start the Qt application)";
    statusElement.className = "ms-font-m disconnected";
  }
}

async function captureScreenshot() {
  if (!serverConnected) {
    showStatus("Please start the Qt Screenshot application first", "error");
    await checkServerConnection();
    return;
  }
  
  try {
    showStatus("Triggering screenshot capture...", "info");
    
    const response = await fetch(`${QT_SERVER_URL}/capture`, {
      method: "POST"
    });
    
    const data = await response.json();
    
    if (data.success) {
      showStatus("Screenshot captured! Waiting for you to select area...", "success");

      setTimeout(() => {
        refreshScreenshots();
      }, 2000);
    } 
    else {
      showStatus("Failed to capture screenshot", "error");
    }
  } 
  catch (error) {
    showStatus("Error communicating with Qt server", "error");
    serverConnected = false;
    await checkServerConnection();
  }
}



async function refreshScreenshots() {
  if (!serverConnected) {
    await checkServerConnection();
    if (!serverConnected) return;
  }
  
  try {
    const response = await fetch(`${QT_SERVER_URL}/screenshots`);
    const data = await response.json();
    const listElement = document.getElementById("screenshots-list");
    const exportBtn = document.getElementById("export-pdf-btn");
    
    console.log("Screenshots response:", data);
    console.log("Number of screenshots:", data.screenshots ? data.screenshots.length : 0);
    
    if (data.success && data.screenshots && data.screenshots.length > 0) {
      listElement.innerHTML = "";
      
      data.screenshots.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));
      
      data.screenshots.forEach(screenshot => {
        const item = createScreenshotItem(screenshot);
        listElement.appendChild(item);
      });

      // Enable export button if there are screenshots
      if (exportBtn) {
        console.log("Enabling export button - screenshots found:", data.screenshots.length);
        exportBtn.disabled = false;
        exportBtn.style.opacity = "1";
      }
    } 
    else {
      listElement.innerHTML = '<p class="ms-font-s">No screenshots available. Take a screenshot to get started!</p>';
      
      // Disable export button if no screenshots
      if (exportBtn) {
        console.log("Disabling export button - no screenshots");
        exportBtn.disabled = true;
      }
    }
  } 
  catch (error) {
    console.error("Error fetching screenshots:", error);
    showStatus("Error loading screenshots", "error");
  }
}


function createScreenshotItem(screenshot) {
  const item = document.createElement("div");
  item.className = "screenshot-item";
  
  const info = document.createElement("div");
  info.className = "screenshot-info";
  
  const id = document.createElement("div");
  id.className = "screenshot-id ms-font-m";
  id.textContent = screenshot.id;
  
  const details = document.createElement("div");
  details.className = "screenshot-details";
  const timestamp = new Date(screenshot.timestamp).toLocaleString();
  details.textContent = `${screenshot.width} × ${screenshot.height} | ${timestamp}`;
  
  info.appendChild(id);
  info.appendChild(details);
  
  item.appendChild(info);
  
  return item;
}


async function exportToPDF() {
  if (!serverConnected) {
    showStatus("Please start the Qt Screenshot application first", "error");
    await checkServerConnection();
    return;
  }
  
  showStatus("Opening save dialog in Qt app...", "info");
  
  try {
    const response = await fetch(`${QT_SERVER_URL}/export-pdf`, {
      method: "POST"
    });
    
    const data = await response.json();
    
    if (data.success) {
      showStatus("✓ Check Qt app to save your PDF!", "success");
      
      // Clear status after 5 seconds
      setTimeout(() => {
        hideStatus();
      }, 5000);
    } else {
      showStatus("Failed to export PDF", "error");
    }
    
  } catch (error) {
    console.error("Error exporting to PDF:", error);
    showStatus("Error communicating with Qt server", "error");
  }
}


function showStatus(message, type) {
  const statusElement = document.getElementById("status-message");
  statusElement.textContent = message;
  statusElement.className = `status-message ${type}`;
  statusElement.style.display = "block";
}


function hideStatus() {
  const statusElement = document.getElementById("status-message");
  statusElement.style.display = "none";
  statusElement.className = "status-message";
}