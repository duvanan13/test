// Function to generate the calendar for a specific year and month
function generateCalendar(year, month) {
    // Get the calendar body element
    var calendarBody = document.getElementById("calendar-body");
  
    // Clear any existing calendar rows
    calendarBody.innerHTML = "";
  
    // Set the current year and month
    currentYear = year;
    currentMonth = month;
  
    // Set the month and year in the header
    var monthYearElement = document.getElementById("month-year");
    monthYearElement.textContent = getMonthName(month) + " " + year;
  
    // Get the number of days in the specified month
    var daysInMonth = new Date(year, month + 1, 0).getDate();
  
    // Get the day of the week for the first day of the month
    var firstDayOfWeek = new Date(year, month, 1).getDay();
  
    // Create the calendar rows and cells
    var date = 1;
    for (var i = 0; i < 6; i++) {
      var row = document.createElement("tr");
  
      for (var j = 0; j < 7; j++) {
        if (i === 0 && j < firstDayOfWeek) {
          var cell = document.createElement("td");
          row.appendChild(cell);
        } else if (date > daysInMonth) {
          break;
        } else {
          var cell = document.createElement("td");
          cell.innerText = date;
          cell.addEventListener("click", function() {
            // Perform action when a date is clicked
            var clickedDate = new Date(currentYear, currentMonth, this.innerText);
            console.log("Clicked date:", clickedDate);
            // Add your desired action here
          });
          row.appendChild(cell);
          date++;
        }
      }
  
      calendarBody.appendChild(row);
    }
  }
  
  // Function to retrieve the month name
  function getMonthName(month) {
    var monthNames = [
      "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"
    ];
    return monthNames[month];
  }
  
  // Function to navigate to the previous month
  function previousMonth() {
    currentMonth--;
    if (currentMonth < 0) {
      currentMonth = 11;
      currentYear--;
    }
    generateCalendar(currentYear, currentMonth);
  }
  
  // Function to navigate to the next month
  function nextMonth() {
    currentMonth++;
    if (currentMonth > 11) {
      currentMonth = 0;
      currentYear++;
    }
    generateCalendar(currentYear, currentMonth);
  }
  
  // Generate the calendar for the current month when the page loads
  var today = new Date();
  var currentYear = today.getFullYear();
  var currentMonth = today.getMonth();
  generateCalendar(currentYear, currentMonth);
  