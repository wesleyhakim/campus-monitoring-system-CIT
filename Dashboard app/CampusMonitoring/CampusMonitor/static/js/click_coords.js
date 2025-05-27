document.addEventListener("DOMContentLoaded", function () {
    const img = document.getElementById("clickable-image");

    if (img) {
        img.addEventListener("click", function (e) {
            const rect = img.getBoundingClientRect();
            const x = ((e.clientX - rect.left) / rect.width) * 100;
            const y = ((e.clientY - rect.top) / rect.height) * 100;

            const x_input = document.getElementById("id_x_coord");
            const y_input = document.getElementById("id_y_coord");

            if (x_input && y_input) {
                x_input.value = x.toFixed(2);  // Round to 2 decimal places
                y_input.value = y.toFixed(2);
            }
        });
    }
});


// document.addEventListener("DOMContentLoaded", function() {
//     const img = document.getElementById("clickable-image");
//     if (img) {
//         img.addEventListener("click", function(event) {
//             const rect = img.getBoundingClientRect();
//             const x = event.clientX - rect.left;
//             const y = event.clientY - rect.top;

//             const xField = document.getElementById("id_x_coord");
//             const yField = document.getElementById("id_y_coord");
//             if (xField && yField) {
//                 xField.value = x.toFixed(2);
//                 yField.value = y.toFixed(2);
//             }
//         });
//     }
// });
