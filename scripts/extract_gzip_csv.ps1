param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [long]$MaxDataRows = 0
)

$inputStream = $null
$gzipStream = $null
$reader = $null
$writer = $null

try {
    $inputStream = [System.IO.File]::OpenRead($InputPath)
    $gzipStream = [System.IO.Compression.GZipStream]::new(
        $inputStream,
        [System.IO.Compression.CompressionMode]::Decompress
    )
    $reader = [System.IO.StreamReader]::new(
        $gzipStream,
        [System.Text.Encoding]::UTF8
    )
    $writer = [System.IO.StreamWriter]::new(
        $OutputPath,
        $false,
        [System.Text.UTF8Encoding]::new($false)
    )

    $header = $reader.ReadLine()
    if ($null -eq $header) {
        throw "El archivo comprimido no contiene una cabecera CSV."
    }
    $writer.WriteLine($header)

    [long]$rows = 0
    while ($MaxDataRows -le 0 -or $rows -lt $MaxDataRows) {
        $line = $reader.ReadLine()
        if ($null -eq $line) {
            break
        }

        $writer.WriteLine($line)
        $rows++

        if ($rows % 1000000 -eq 0) {
            Write-Host "  Filas descomprimidas: $rows"
        }
    }

    Write-Host "Archivo creado con $rows filas de datos: $OutputPath"
}
finally {
    if ($null -ne $writer) { $writer.Dispose() }
    if ($null -ne $reader) { $reader.Dispose() }
    if ($null -ne $gzipStream) { $gzipStream.Dispose() }
    if ($null -ne $inputStream) { $inputStream.Dispose() }
}
