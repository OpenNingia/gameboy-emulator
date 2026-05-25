<#
.SYNOPSIS
    Riscrive autore e committer di TUTTI i commit del repo a "Daniele <oppifjellet@gmail.com>".

.DESCRIPTION
    Usa git-filter-repo (https://github.com/newren/git-filter-repo) per riscrivere
    author/committer (sia name che email) di tutti i commit su tutti i ref.

    ATTENZIONE: questa operazione e' DISTRUTTIVA.
      - Cambia gli SHA di tutti i commit.
      - Richiede un force-push per propagare su 'origin'.
      - filter-repo rimuove il remote 'origin' a fine operazione (safety net):
        va riaggiunto manualmente prima del push.
      - Tutti i collaboratori dovranno riclonare (o fare reset --hard).

    Lo script crea un backup ref prima di procedere, cosi' in caso di errore
    si puo' tornare allo stato precedente con:
        git update-ref refs/heads/main refs/backup/pre-author-rewrite-<TS>

.PARAMETER NewName
    Nome autore/committer di destinazione. Default: "Daniele".

.PARAMETER NewEmail
    Email autore/committer di destinazione. Default: "oppifjellet@gmail.com".

.PARAMETER Force
    Necessario per eseguire davvero la riscrittura. Senza -Force lo script
    fa solo dry-run / stampa il piano.

.EXAMPLE
    # Dry-run: mostra cosa farebbe
    .\scripts\rewrite-authors.ps1

.EXAMPLE
    # Esecuzione effettiva
    .\scripts\rewrite-authors.ps1 -Force
#>

[CmdletBinding()]
param(
    [string]$NewName  = 'Daniele',
    [string]$NewEmail = 'oppifjellet@gmail.com',
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Fail($msg) {
    Write-Host "ERROR: $msg" -ForegroundColor Red
    exit 1
}

# --- Pre-flight ----------------------------------------------------------

# Working dir = git repo root
$repoRoot = (git rev-parse --show-toplevel 2>$null).Trim()
if (-not $repoRoot) { Fail "Non sembra un repo git." }
Set-Location $repoRoot
Write-Host "Repo: $repoRoot"

# Working tree pulito? (gli untracked sono OK, filter-repo non li tocca)
$dirty = git status --porcelain --untracked-files=no
if ($dirty) {
    Write-Host "Working tree non pulito (modifiche tracked):" -ForegroundColor Yellow
    Write-Host $dirty
    Fail "Commit/stash le modifiche prima di riscrivere la history."
}

# git-filter-repo invocabile? Prova prima come git subcommand, poi come modulo python.
# Su Windows con `pip install --user`, l'eseguibile spesso non e' in PATH e git fallisce
# con "cannot spawn git-filter-repo"; in quel caso il modulo python funziona comunque.
$filterRepoCmd = $null
$null = git filter-repo --version 2>&1
if ($LASTEXITCODE -eq 0) {
    $filterRepoCmd = @('git', 'filter-repo')
} else {
    $null = python -m git_filter_repo --version 2>&1
    if ($LASTEXITCODE -eq 0) {
        $filterRepoCmd = @('python', '-m', 'git_filter_repo')
        Write-Host "Uso 'python -m git_filter_repo' (git non trova l'eseguibile sul PATH)." -ForegroundColor DarkGray
    }
}
if (-not $filterRepoCmd) {
    Write-Host "git-filter-repo non e' invocabile." -ForegroundColor Yellow
    Write-Host "Installa con:" -ForegroundColor Yellow
    Write-Host "    pip install git-filter-repo" -ForegroundColor Cyan
    Write-Host "Su Windows, se 'git filter-repo' fallisce con 'cannot spawn', verifica che" -ForegroundColor Yellow
    Write-Host "%APPDATA%\Python\Python<ver>\Scripts sia in PATH, oppure usa 'python -m git_filter_repo'." -ForegroundColor Yellow
    Fail "Dipendenza mancante."
}

# --- Piano ---------------------------------------------------------------

$currentAuthors = git log --all --format='%an <%ae>' | Sort-Object -Unique
Write-Host ""
Write-Host "Identita' attuali nello storico:" -ForegroundColor Cyan
$currentAuthors | ForEach-Object { Write-Host "  - $_" }

Write-Host ""
Write-Host "Tutti i commit verranno riscritti a:" -ForegroundColor Cyan
Write-Host "  $NewName <$NewEmail>"
Write-Host ""

if (-not $Force) {
    Write-Host "DRY-RUN: nessuna modifica eseguita. Rilancia con -Force per procedere." -ForegroundColor Yellow
    exit 0
}

# --- Backup ref ----------------------------------------------------------

$ts = Get-Date -Format 'yyyyMMdd-HHmmss'
$backupRef = "refs/backup/pre-author-rewrite-$ts"
$head = (git rev-parse HEAD).Trim()
git update-ref $backupRef $head
if ($LASTEXITCODE -ne 0) { Fail "Creazione backup ref fallita." }
Write-Host "Backup creato: $backupRef -> $head" -ForegroundColor Green

# --- Rewrite -------------------------------------------------------------

# Callback in bytes (filter-repo passa bytes). Le doppie virgolette sono
# fondamentali per la PowerShell -> python passthrough.
$nameCb  = "return b`"$NewName`""
$emailCb = "return b`"$NewEmail`""

Write-Host ""
Write-Host ("Eseguo {0}..." -f ($filterRepoCmd -join ' ')) -ForegroundColor Cyan
& $filterRepoCmd[0] @($filterRepoCmd[1..($filterRepoCmd.Length - 1)]) `
    --force `
    --name-callback  $nameCb `
    --email-callback $emailCb

if ($LASTEXITCODE -ne 0) { Fail "filter-repo ha fallito. Lo stato precedente e' in $backupRef." }

# --- Post ----------------------------------------------------------------

Write-Host ""
Write-Host "Riscrittura completata." -ForegroundColor Green
Write-Host ""
Write-Host "Identita' presenti adesso:" -ForegroundColor Cyan
git log --all --format='%an <%ae>' | Sort-Object -Unique | ForEach-Object { Write-Host "  - $_" }

Write-Host ""
Write-Host "Prossimi passi:" -ForegroundColor Yellow
Write-Host "  1. filter-repo ha rimosso 'origin' (safety). Riaggiungilo:"
Write-Host "       git remote add origin git@github.com:OpenNingia/gameboy-emulator.git" -ForegroundColor Cyan
Write-Host "  2. Verifica gli SHA / il log."
Write-Host "  3. Force-push (DISTRUTTIVO per i collaboratori):"
Write-Host "       git push --force --all origin"     -ForegroundColor Cyan
Write-Host "       git push --force --tags origin"    -ForegroundColor Cyan
Write-Host "  4. Backup ref disponibile in $backupRef (per rollback)."
