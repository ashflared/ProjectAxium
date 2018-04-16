#include "AuthCodes.h"

namespace AuthHelper
{
    static RealmBuildInfo const PostBcAcceptedClientBuilds[] =
    {
        { 13930,    3,  3,  5,  'b' },
        { 12340,    3,  3,  5,  'a' },
        { 11723,    3,  3,  3,  'a' },
        { 11403,    3,  3,  2,  ' ' },
        { 11159,    3,  3,  0,  'a' },
        { 10505,    3,  2,  2,  'a' },
        { 9947,     3,  1,  3,  ' ' },
        { 8606,     2,  4,  3,  ' ' },
        { 0,        0,  0,  0,  ' ' }
    };

    static RealmBuildInfo const PreBcAcceptedClientBuilds[] =
    {
        { 6141,     1,  12, 3,  ' ' },
        { 6005,     1,  12, 2,  ' ' },
        { 5875,     1,  12, 1,  ' ' },
        { 0,        0,  0,  0,  ' ' }
    };

    bool IsPreBCAcceptedClientBuild(int build)
    {
        for (int i = 0; PreBcAcceptedClientBuilds[i].Build; ++i)
            if (PreBcAcceptedClientBuilds[i].Build == build)
                return true;

        return false;
    }

    bool IsPostBCAcceptedClientBuild(int build)
    {
        for (int i = 0; PostBcAcceptedClientBuilds[i].Build; ++i)
            if (PostBcAcceptedClientBuilds[i].Build == build)
                return true;

        return false;
    }

    bool IsAcceptedClientBuild(int build)
    {
        return (IsPostBCAcceptedClientBuild(build) || IsPreBCAcceptedClientBuild(build));
    }

    RealmBuildInfo const* GetBuildInfo(int build)
    {
        for (int i = 0; PostBcAcceptedClientBuilds[i].Build; ++i)
            if (PostBcAcceptedClientBuilds[i].Build == build)
                return &PostBcAcceptedClientBuilds[i];

        for (int i = 0; PreBcAcceptedClientBuilds[i].Build; ++i)
            if (PreBcAcceptedClientBuilds[i].Build == build)
                return &PreBcAcceptedClientBuilds[i];

        return NULL;
    }
};
