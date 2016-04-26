#include "StdAfx.h"
#include "Framebuffer.h"

#include "glog\logging.h"

namespace billiard {
    bool isFramebufferOk(GLenum err) {
        switch (err) {
            case GL_FRAMEBUFFER_COMPLETE_EXT:
                return true;

            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOG(ERROR) << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                LOG(ERROR) << "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                LOG(ERROR) << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
                break;

            case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                LOG(ERROR) << "GL_FRAMEBUFFER_UNSUPPORTED";
                break;
        }
        return false;
    }
}