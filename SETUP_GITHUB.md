# Setting Up GitHub Repository

## Pre-commit with Gitleaks

The repository includes `.pre-commit-config.yaml` with gitleaks configured. To set it up:

```bash
# Install pre-commit (if not already installed)
pip install --user pre-commit
# or
pip3 install --user pre-commit

# Install the git hooks
pre-commit install

# Run gitleaks manually to check for secrets
pre-commit run gitleaks --all-files
```

## Creating the GitHub Repository

### Option 1: Using GitHub CLI (if installed)

```bash
gh repo create lunar-telescope --public --source=. --remote=origin --push
```

### Option 2: Manual Creation

1. Go to https://github.com/new
2. Repository name: `lunar-telescope`
3. Description: "Waypipe-based remote application framework with predictive input and performance optimization"
4. Set to **Public**
5. **Do NOT** initialize with README, .gitignore, or license (we already have these)
6. Click "Create repository"

Then push the code:

```bash
git remote add origin https://github.com/YOUR_USERNAME/lunar-telescope.git
git push -u origin main
```

### Option 3: Using SSH

```bash
git remote add origin git@github.com:YOUR_USERNAME/lunar-telescope.git
git push -u origin main
```

## Verify Pre-commit Works

After pushing, you can verify gitleaks is working:

```bash
# Install pre-commit hooks locally
pre-commit install

# Test on next commit
echo "test" >> test.txt
git add test.txt
git commit -m "test"  # Should trigger gitleaks
```

## CI/CD

The repository includes `.github/workflows/ci.yml` which will run on push/PR. It includes:
- Build verification
- Linting
- Documentation checks

Gitleaks will also run in CI if configured in the workflow.

