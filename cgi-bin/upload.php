#!/usr/bin/env php
<?php
echo "Content-Type: text/html\r\n\r\n";
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
echo "<html><body>";
echo "<h1>File Upload Manager (PHP)</h1>";
echo "<p>Method: $method</p>";

// Create uploads directory if it doesn't exist
$upload_dir = './uploads/';
if (!is_dir($upload_dir)) {
    mkdir($upload_dir, 0755, true);
}

// Function to clear all contents from directory but keep the directory
function clearDirectory($dir) {
    if (!is_dir($dir)) {
        return false;
    }
    
    $files = array_diff(scandir($dir), array('.', '..'));
    foreach ($files as $file) {
        $path = $dir . DIRECTORY_SEPARATOR . $file;
        if (is_dir($path)) {
            deleteDirectory($path); // Remove subdirectories completely
        } else {
            unlink($path); // Remove files
        }
    }
    return true;
}

// Helper function to recursively delete subdirectories
function deleteDirectory($dir) {
    if (!is_dir($dir)) {
        return false;
    }
    
    $files = array_diff(scandir($dir), array('.', '..'));
    foreach ($files as $file) {
        $path = $dir . DIRECTORY_SEPARATOR . $file;
        if (is_dir($path)) {
            deleteDirectory($path);
        } else {
            unlink($path);
        }
    }
    return rmdir($dir);
}

if ($method === 'GET') {
    // Show upload form and existing files
    echo "<h2>Upload New File</h2>";
    echo '<form action="upload.php" method="post" enctype="multipart/form-data">';
    echo '<input type="file" name="uploadfile" required><br><br>';
    echo '<input type="text" name="description" placeholder="File description"><br><br>';
    echo '<input type="submit" value="Upload File">';
    echo '</form>';
    
    // Console POST instructions
    // echo "<h2>Console POST Instructions</h2>";
    // echo "<div style='background-color:#f0f8ff;border:1px solid #4a90e2;padding:10px;margin:10px 0;'>";
    // echo "<h4>To upload via console POST, use this format:</h4>";
    // echo "<pre>filename=test.txt&content=Hello World!&description=Test file</pre>";
    // echo "<p><strong>Or for JSON:</strong></p>";
    // echo '<pre>{"filename":"test.json","content":"{\\"key\\":\\"value\\"}","description":"JSON file"}</pre>';
    // echo "</div>";
    
    echo "<h2>Existing Files</h2>";
    if (is_dir($upload_dir)) {
        $files = glob($upload_dir . '*');
        if ($files) {
            echo "<table border='1'>";
            echo "<tr><th>File</th><th>Size</th><th>Modified</th><th>Actions</th></tr>";
            foreach ($files as $file) {
                $filename = basename($file);
                $size = filesize($file);
                $modified = date("Y-m-d H:i:s", filemtime($file));
                echo "<tr>";
                echo "<td><a href='/uploads/$filename'>$filename</a></td>";
                echo "<td>$size bytes</td>";
                echo "<td>$modified</td>";
                echo "<td><a href='upload.php?delete=$filename'>Delete File</a></td>";
                echo "</tr>";
            }
            echo "</table>";
            
            // Add button to clear all files from uploads folder
            echo "<br><div style='background-color:#ffeeee;border:1px solid red;padding:10px;margin:10px 0;'>";
            echo "<h3 style='color:red'>⚠️ DANGER ZONE</h3>";
            echo "<p><strong>Warning:</strong> This will delete ALL files in the uploads directory!</p>";
            echo "<form method='post' onsubmit='return confirm(\"Are you sure you want to delete ALL files in the uploads directory? This cannot be undone!\");'>";
            echo '<input type="hidden" name="clear_all" value="1">';
            echo '<input type="submit" value="🗑️ CLEAR ALL FILES" style="background-color:red;color:white;font-weight:bold;padding:10px;">';
            echo "</form>";
            echo "</div>";
        } else {
            echo "<p>No files uploaded yet.</p>";
        }
    } else {
        echo "<p>Uploads directory does not exist.</p>";
    }
    
// } elseif ($method === 'POST') {
//     // Check if this is a request to clear all files
//     if (isset($_POST['clear_all'])) {
//         echo "<div style='background-color:#ffeeee;border:1px solid red;padding:10px;margin:10px 0;'>";
//         echo "<h3 style='color:red'>🗑️ CLEARING ALL FILES FROM UPLOADS DIRECTORY</h3>";
        
//         if (is_dir($upload_dir)) {
//             if (clearDirectory($upload_dir)) {
//                 echo "<p style='color:green;font-weight:bold'>✅ All files cleared from uploads directory successfully!</p>";
//                 echo "<p>The uploads directory is now empty but still exists.</p>";
//             } else {
//                 echo "<p style='color:red;font-weight:bold'>❌ Error clearing files from uploads directory!</p>";
//             }
//         } else {
//             echo "<p style='color:orange'>⚠️ Uploads directory doesn't exist.</p>";
//             mkdir($upload_dir, 0755, true);
//             echo "<p style='color:green'>Created new uploads directory.</p>";
//         }
//         echo "</div>";
//     }
//     // Handle regular multipart file upload
//     elseif (isset($_FILES['uploadfile']) && $_FILES['uploadfile']['error'] === UPLOAD_ERR_OK) {
//         // Recreate uploads directory if it was deleted
//         if (!is_dir($upload_dir)) {
//             mkdir($upload_dir, 0755, true);
//         }
        
//         $tmp_name = $_FILES['uploadfile']['tmp_name'];
//         $filename = basename($_FILES['uploadfile']['name']);
//         $description = $_POST['description'] ?? '';
//         $upload_path = $upload_dir . $filename;
        
//         if (move_uploaded_file($tmp_name, $upload_path)) {
//             echo "<p style='color:green'>File '$filename' uploaded successfully!</p>";
//             echo "<p>Description: $description</p>";
            
//             // Show file info
//             $size = filesize($upload_path);
//             $type = $_FILES['uploadfile']['type'];
//             echo "<p>Size: $size bytes</p>";
//             echo "<p>Type: $type</p>";
            
//             // If it's an image, show preview
//             if (strpos($type, 'image/') === 0) {
//                 echo "<img src='/uploads/$filename' style='max-width:400px;' alt='Uploaded image'>";
//             }
//         } else {
//             echo "<p style='color:red'>Error uploading file!</p>";
//         }
//     }
//     // NEW: Handle raw POST data from console
//     elseif (!empty($raw_input = file_get_contents('php://input'))) {
//         echo "<h3>Console POST Data Received</h3>";
//         echo "<p style='color:blue'>Processing console upload...</p>";
        
//         // Recreate uploads directory if it was deleted
//         if (!is_dir($upload_dir)) {
//             mkdir($upload_dir, 0755, true);
//         }
        
//         // Get content type to determine how to parse
//         $content_type = $_SERVER['CONTENT_TYPE'] ?? '';
        
//         if (strpos($content_type, 'application/json') !== false) {
//             // Handle JSON data
//             $json_data = json_decode($raw_input, true);
            
//             if (json_last_error() === JSON_ERROR_NONE && isset($json_data['filename']) && isset($json_data['content'])) {
//                 $filename = basename($json_data['filename']);
//                 $content = $json_data['content'];
//                 $description = $json_data['description'] ?? '';
//                 $upload_path = $upload_dir . $filename;
                
//                 if (file_put_contents($upload_path, $content)) {
//                     echo "<p style='color:green'>File '$filename' created successfully via JSON POST!</p>";
//                     echo "<p>Description: $description</p>";
//                     echo "<p>Size: " . strlen($content) . " bytes</p>";
//                     echo "<p>Content type: JSON</p>";
//                 } else {
//                     echo "<p style='color:red'>Error creating file '$filename'!</p>";
//                 }
//             } else {
//                 echo "<p style='color:red'>Invalid JSON data or missing filename/content fields!</p>";
//                 echo "<p>Expected format: {\"filename\":\"name.ext\",\"content\":\"file content\",\"description\":\"optional\"}</p>";
//             }
//         } else {
//             // Handle form-encoded data (default)
//             parse_str($raw_input, $post_data);
            
//             if (isset($post_data['filename']) && isset($post_data['content'])) {
//                 $filename = basename($post_data['filename']);
//                 $content = $post_data['content'];
//                 $description = $post_data['description'] ?? '';
//                 $upload_path = $upload_dir . $filename;
                
//                 if (file_put_contents($upload_path, $content)) {
//                     echo "<p style='color:green'>File '$filename' created successfully via console POST!</p>";
//                     echo "<p>Description: $description</p>";
//                     echo "<p>Size: " . strlen($content) . " bytes</p>";
//                     echo "<p>Content type: Form data</p>";
                    
//                     // Show content preview for text files
//                     if (strlen($content) < 500) {
//                         echo "<h4>Content preview:</h4>";
//                         echo "<pre style='background-color:#f5f5f5;padding:10px;border:1px solid #ccc;'>" . htmlspecialchars($content) . "</pre>";
//                     }
//                 } else {
//                     echo "<p style='color:red'>Error creating file '$filename'!</p>";
//                 }
//             } else {
//                 echo "<p style='color:orange'>Raw POST data received but no filename/content found:</p>";
//                 echo "<pre style='background-color:#fff3cd;padding:10px;border:1px solid #ffeaa7;'>" . htmlspecialchars($raw_input) . "</pre>";
//                 echo "<p><strong>Expected format:</strong> filename=test.txt&content=Hello World&description=optional</p>";
//                 echo "<p><strong>Or JSON:</strong> {\"filename\":\"test.txt\",\"content\":\"Hello World\",\"description\":\"optional\"}</p>";
//             }
//         }
//     } else {
//         echo "<p style='color:red'>No file uploaded or POST data received.</p>";
//     }
} elseif ($method === 'POST') {
    $raw_input = file_get_contents('php://input');
    echo "<div style='border:2px solid red; padding:10px;'>";
    echo "<h3>DEBUG INFO:</h3>";
    echo "<p>Raw input length: " . strlen($raw_input) . "</p>";
    echo "<p>Raw input content: " . htmlspecialchars($raw_input) . "</p>";
    echo "<p>POST array: " . print_r($_POST, true) . "</p>";
    echo "<p>FILES array: " . print_r($_FILES, true) . "</p>";
    echo "</div>";
	

    
} elseif ($method === 'DELETE' || isset($_GET['delete'])) {
    // Handle individual file deletion or entire folder deletion
    $filename = $_GET['delete'] ?? '';
    
    // Check if this is a request to clear the entire folder
    if ($filename === 'ALL' || empty($filename)) {
        echo "<div style='background-color:#ffeeee;border:1px solid red;padding:10px;margin:10px 0;'>";
        echo "<h3 style='color:red'>🗑️ CLEARING ALL FILES FROM UPLOADS DIRECTORY</h3>";
        
        if (is_dir($upload_dir)) {
            if (clearDirectory($upload_dir)) {
                echo "<p style='color:green;font-weight:bold'>✅ All files cleared from uploads directory successfully!</p>";
                echo "<p>The uploads directory is now empty but still exists.</p>";
            } else {
                echo "<p style='color:red;font-weight:bold'>❌ Error clearing files from uploads directory!</p>";
            }
        } else {
            echo "<p style='color:orange'>⚠️ Uploads directory doesn't exist.</p>";
            mkdir($upload_dir, 0755, true);
            echo "<p style='color:green'>Created new uploads directory.</p>";
        }
        echo "</div>";
    } else {
        // Handle individual file deletion
        $filepath = $upload_dir . basename($filename); // Sanitize filename
        if (file_exists($filepath)) {
            if (unlink($filepath)) {
                echo "<p style='color:green'>File '$filename' deleted successfully!</p>";
            } else {
                echo "<p style='color:red'>Error deleting file '$filename'!</p>";
            }
        } else {
            echo "<p style='color:red'>File '$filename' not found!</p>";
        }
    }
} else {
    echo "<p>Unsupported HTTP method: $method</p>";
}

echo "<br><a href='upload.php'>🔄 Back to Upload Manager</a>";
echo "</body></html>";
?>