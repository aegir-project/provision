# Use this Dockerfile for building a new image with your desired UID.
ARG IMAGE_NAME=http
ARG IMAGE_TAG=php7

FROM provision4/$IMAGE_NAME:$IMAGE_TAG

ENV IMAGE_TAG ${IMAGE_TAG:-php7}
ENV IMAGE_NAME ${IMAGE_NAME:-http}

# The UID and GID for the USER_NAME user.
ARG NEW_UID=3333
ENV NEW_UID ${NEW_UID:-3333}

ARG NEW_GID=3333
ENV NEW_GID ${NEW_GID:-3333}

ENV USER_NAME provision

USER root

RUN echo "Hi! Attempting to update user $USER_NAME to UID $NEW_UID and GID $NEW_GID..."
RUN usermod -u $NEW_UID $USER_NAME
RUN groupmod -g $NEW_GID $USER_NAME

RUN echo "Attempting to change ownership of /var/$USER_NAME to $USER_NAME:$USER_NAME ..."
RUN chown $USER_NAME:$USER_NAME /var/$USER_NAME -R

USER $USER_NAME