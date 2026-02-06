# Rommlet Development Container
# Based on devkitPro's official devkitARM image for 3DS development

FROM devkitpro/devkitarm:latest

# Install additional tools needed for the build
RUN apt-get update && apt-get install -y --no-install-recommends \
    clang-format \
    librsvg2-bin \
    git \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install bannertool and makerom (3DS tools)
# These are needed to create CIA files
RUN mkdir -p /opt/3ds-tools && \
    cd /opt/3ds-tools && \
    # Download bannertool
    wget -q https://github.com/Epicpkmn11/bannertool/releases/download/v1.2.2/bannertool.zip && \
    unzip -q bannertool.zip && \
    cp linux-x86_64/bannertool /usr/local/bin/ && \
    chmod +x /usr/local/bin/bannertool && \
    rm -rf bannertool.zip linux-x86_64 && \
    # Download makerom
    wget -q https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.4/makerom-v0.18.4-ubuntu_x86_64.zip && \
    unzip -q makerom-v0.18.4-ubuntu_x86_64.zip && \
    cp makerom /usr/local/bin/ && \
    chmod +x /usr/local/bin/makerom && \
    rm -rf makerom-v0.18.4-ubuntu_x86_64.zip makerom && \
    cd / && rm -rf /opt/3ds-tools

# Create non-root user for VS Code
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

# Set environment variables for devkitPro
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=$PATH:$DEVKITARM/bin:$DEVKITPRO/tools/bin

# Set working directory
WORKDIR /workspace

USER $USERNAME
