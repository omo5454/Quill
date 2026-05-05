# add this to your path and it will execute the quill.js file on the given paths
param (
    [Parameter(Mandatory=$true, ValueFromRemainingArguments=$true)]
    [string[]]$Paths
)

foreach ($path in $Paths) {
    # More stable version of the execution line
    (node "$env:USERPROFILE/projects/interperter/dist/quill.js" $path)
}
