
## Coding guidelines

### C/C++ coding style

Changes should conform to [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

We use `clang-format-14` to ensure consistent code formatting. To install it on Ubuntu, use the following command:
```sh
apt-get install -y clang-format-14
```

### Pre-Commit Hooks

To automate code linting and verification, install the pre-commit hooks from the top-level folder of the repo:
```sh
pre-commit install -c .pre-commit-config.yaml
```

This ensures that your changes meet our coding standards before being committed.

Assumming your `PWD` is in the root of the repository, you can manually trigger the hooks at any time by running from the top-level folder of the repo:
```sh
pre-commit run -a -c .pre-commit-config.yaml
```
