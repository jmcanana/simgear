// Copyright (C) 2013  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear_config.h>
#include <simgear/package/Catalog.hxx>

#include <boost/foreach.hpp>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/misc/strutils.hxx>

namespace simgear {

namespace pkg {

bool checkVersionString(const std::string& aVersion, const std::string& aCandidate)
{
    if (aCandidate == aVersion) {
        return true;
    }

    // examine each dot-seperated component in turn, supporting a wildcard
    // in the versions from the catalog.
    string_list appVersionParts = simgear::strutils::split(aVersion, ".");
    string_list catVersionParts = simgear::strutils::split(aCandidate, ".");

    size_t partCount = appVersionParts.size();
    bool previousCandidatePartWasWildcard = false;

    for (unsigned int p=0; p < partCount; ++p) {
        // candidate string is too short, can match if it ended with wildcard
        // part. This allows candidate '2016.*' to match '2016.1.2' and so on
        if (catVersionParts.size() <= p) {
            return previousCandidatePartWasWildcard;
        }

        if (catVersionParts[p] == "*") {
            // always passes
            previousCandidatePartWasWildcard = true;
        } else if (appVersionParts[p] != catVersionParts[p]) {
            return false;
        }
    }

    return true;
}

bool checkVersion(const std::string& aVersion, SGPropertyNode_ptr props)
{
    BOOST_FOREACH(SGPropertyNode* v, props->getChildren("version")) {
        std::string s(v->getStringValue());
        if (checkVersionString(aVersion, s)) {
            return true;
        }
    }
    return false;
}

std::string redirectUrlForVersion(const std::string& aVersion, SGPropertyNode_ptr props)
{
    BOOST_FOREACH(SGPropertyNode* v, props->getChildren("alternate-version")) {
        std::string s(v->getStringValue("version"));
        if (checkVersionString(aVersion, s)) {
            return  v->getStringValue("url");;
        }
    }

    return std::string();
}

//////////////////////////////////////////////////////////////////////////////

class Catalog::Downloader : public HTTP::Request
{
public:
    Downloader(CatalogRef aOwner, const std::string& aUrl) :
        HTTP::Request(aUrl),
        m_owner(aOwner)
    {
        // refreshing
        m_owner->changeStatus(Delegate::STATUS_IN_PROGRESS);
    }

protected:
    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
    }

    virtual void onDone()
    {
        if (responseCode() != 200) {
            Delegate::StatusCode code = Delegate::FAIL_DOWNLOAD;
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog download failure:" << m_owner->url()
                   << "\n\t" << responseCode());
            if (responseCode() == 404) {
                code = Delegate::FAIL_NOT_FOUND;
            }
            m_owner->refreshComplete(code);
            return;
        }

        SGPropertyNode_ptr props = new SGPropertyNode;

        try {
            readProperties(m_buffer.data(), m_buffer.size(), props);
            m_owner->parseProps(props);
        } catch (sg_exception& ) {
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog parse failure:" << m_owner->url());
            m_owner->refreshComplete(Delegate::FAIL_EXTRACT);
            return;
        }

        std::string ver(m_owner->root()->applicationVersion());
        if (!checkVersion(ver, props)) {
            SG_LOG(SG_GENERAL, SG_WARN, "downloaded catalog " << m_owner->url() << ", version required " << ver);

            // check for a version redirect entry
            std::string url = redirectUrlForVersion(ver, props);
            if (!url.empty()) {
                SG_LOG(SG_GENERAL, SG_WARN, "redirecting from " << m_owner->url() <<
                       " to \n\t" << url);

                // update the URL and kick off a new request
                m_owner->setUrl(url);
                Downloader* dl = new Downloader(m_owner, url);
                m_owner->root()->makeHTTPRequest(dl);
            } else {
                m_owner->refreshComplete(Delegate::FAIL_VERSION);
            }

            return;
        } // of version check failed

        // cache the catalog data, now we have a valid install root
        Dir d(m_owner->installRoot());
        SGPath p = d.file("catalog.xml");
        sg_ofstream f(p, std::ios::out | std::ios::trunc);
        f.write(m_buffer.data(), m_buffer.size());
        f.close();

        time(&m_owner->m_retrievedTime);
        m_owner->writeTimestamp();
        m_owner->refreshComplete(Delegate::STATUS_REFRESHED);
    }

private:

    CatalogRef m_owner;
    std::string m_buffer;
};

//////////////////////////////////////////////////////////////////////////////

Catalog::Catalog(Root *aRoot) :
    m_root(aRoot),
    m_status(Delegate::FAIL_UNKNOWN),
    m_retrievedTime(0)
{
}

Catalog::~Catalog()
{
}

CatalogRef Catalog::createFromUrl(Root* aRoot, const std::string& aUrl)
{
    CatalogRef c = new Catalog(aRoot);
    c->m_url = aUrl;
    c->refresh();
    return c;
}

CatalogRef Catalog::createFromPath(Root* aRoot, const SGPath& aPath)
{
    SGPath xml = aPath;
    xml.append("catalog.xml");
    if (!xml.exists()) {
        return NULL;
    }

    SGPropertyNode_ptr props;
    try {
        props = new SGPropertyNode;
        readProperties(xml, props);
    } catch (sg_exception& ) {
        return NULL;
    }

    bool versionCheckOk = checkVersion(aRoot->applicationVersion(), props);
    if (!versionCheckOk) {
        SG_LOG(SG_GENERAL, SG_INFO, "catalog at:" << aPath << " failed version check: need" << aRoot->applicationVersion());
        // keep the catalog but mark it as needing an update
    } else {
        SG_LOG(SG_GENERAL, SG_DEBUG, "creating catalog from:" << aPath);
    }

    CatalogRef c = new Catalog(aRoot);
    c->m_installRoot = aPath;
    c->parseProps(props);
    c->parseTimestamp();

    if (versionCheckOk) {
        // parsed XML ok, mark status as valid
        c->changeStatus(Delegate::STATUS_SUCCESS);
    } else {
        c->changeStatus(Delegate::FAIL_VERSION);
    }

    return c;
}

bool Catalog::uninstall()
{
    bool ok;
    bool atLeastOneFailure = false;

    BOOST_FOREACH(PackageRef p, installedPackages()) {
        ok = p->existingInstall()->uninstall();
        if (!ok) {
            SG_LOG(SG_GENERAL, SG_WARN, "uninstall of package " <<
                p->id() << " failed");
            // continue trying other packages, bailing out here
            // gains us nothing
            atLeastOneFailure = true;
        }
    }

    Dir d(m_installRoot);
    ok = d.remove(true /* recursive */);
    if (!ok) {
        atLeastOneFailure = true;
    }

    changeStatus(atLeastOneFailure ? Delegate::FAIL_FILESYSTEM
                                   : Delegate::STATUS_SUCCESS);
    return ok;
}

PackageList const&
Catalog::packages() const
{
  return m_packages;
}

PackageList
Catalog::packagesMatching(const SGPropertyNode* aFilter) const
{
    PackageList r;
    BOOST_FOREACH(PackageRef p, m_packages) {
        if (p->matches(aFilter)) {
            r.push_back(p);
        }
    }
    return r;
}

PackageList
Catalog::packagesNeedingUpdate() const
{
    PackageList r;
    BOOST_FOREACH(PackageRef p, m_packages) {
        if (!p->isInstalled()) {
            continue;
        }

        if (p->install()->hasUpdate()) {
            r.push_back(p);
        }
    }
    return r;
}

PackageList
Catalog::installedPackages() const
{
  PackageList r;
  BOOST_FOREACH(PackageRef p, m_packages) {
    if (p->isInstalled()) {
      r.push_back(p);
    }
  }
  return r;
}

void Catalog::refresh()
{
    if (m_refreshRequest.valid()) {
        // refresh in progress
        return;
    }

    Downloader* dl = new Downloader(this, url());
    m_refreshRequest = dl;
    // will update status to IN_PROGRESS
    m_root->makeHTTPRequest(dl);
}

struct FindById
{
    FindById(const std::string &id) : m_id(id) {}

    bool operator()(const PackageRef& ref) const
    {
        return ref->id() == m_id;
    }

    std::string m_id;
};

void Catalog::parseProps(const SGPropertyNode* aProps)
{
    // copy everything except package children?
    m_props = new SGPropertyNode;

    m_variantDict.clear(); // will rebuild during parse
    std::set<PackageRef> orphans;
    orphans.insert(m_packages.begin(), m_packages.end());

    int nChildren = aProps->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* pkgProps = aProps->getChild(i);
        if (strcmp(pkgProps->getName(), "package") == 0) {
            // can't use getPackageById here becuase the variant dict isn't
            // built yet. Instead we need to look at m_packages directly.

            PackageList::iterator pit = std::find_if(m_packages.begin(), m_packages.end(),
                                                  FindById(pkgProps->getStringValue("id")));
            PackageRef p;
            if (pit != m_packages.end()) {
                p = *pit;
                // existing package
                p->updateFromProps(pkgProps);
                orphans.erase(p); // not an orphan
            } else {
                // new package
                p = new Package(pkgProps, this);
                m_packages.push_back(p);
            }

            string_list vars(p->variants());
            for (string_list::iterator it = vars.begin(); it != vars.end(); ++it) {
                m_variantDict[*it] = p.ptr();
            }
        } else {
            SGPropertyNode* c = m_props->getChild(pkgProps->getName(), pkgProps->getIndex(), true);
            copyProperties(pkgProps, c);
        }
    } // of children iteration

    if (!orphans.empty()) {
        SG_LOG(SG_GENERAL, SG_WARN, "have orphan packages: will become inaccesible");
        std::set<PackageRef>::iterator it;
        for (it = orphans.begin(); it != orphans.end(); ++it) {
            SG_LOG(SG_GENERAL, SG_WARN, "\torphan package:" << (*it)->qualifiedId());
            PackageList::iterator pit = std::find(m_packages.begin(), m_packages.end(), *it);
            assert(pit != m_packages.end());
            m_packages.erase(pit);
        }
    }

    if (!m_url.empty()) {
        if (m_url != m_props->getStringValue("url")) {
            // this effectively allows packages to migrate to new locations,
            // although if we're going to rely on that feature we should
            // maybe formalise it!
            SG_LOG(SG_GENERAL, SG_WARN, "package downloaded from:" << m_url
                   << " is now at: " << m_props->getStringValue("url"));
        }
    }

    m_url = m_props->getStringValue("url");

    if (m_installRoot.isNull()) {
        m_installRoot = m_root->path();
        m_installRoot.append(id());

        Dir d(m_installRoot);
        d.create(0755);
    }
}

PackageRef Catalog::getPackageById(const std::string& aId) const
{
    // search the variant dict here, so looking up aircraft variants
    // works as expected.
    PackageWeakMap::const_iterator it = m_variantDict.find(aId);
    if (it == m_variantDict.end())
        return PackageRef();

    return it->second;
}

PackageRef Catalog::getPackageByPath(const std::string& aPath) const
{
    PackageList::const_iterator it;
    for (it = m_packages.begin(); it != m_packages.end(); ++it) {
        if ((*it)->dirName() == aPath) {
            return *it;
        }
    }

    return PackageRef();
}

std::string Catalog::id() const
{
    return m_props->getStringValue("id");
}

std::string Catalog::url() const
{
    return m_url;
}

void Catalog::setUrl(const std::string &url)
{
    m_url = url;
    if (m_status == Delegate::FAIL_NOT_FOUND) {
        m_status = Delegate::FAIL_UNKNOWN;
    }
}

std::string Catalog::name() const
{
    return getLocalisedString(m_props, "name");
}

std::string Catalog::description() const
{
    return getLocalisedString(m_props, "description");
}

SGPropertyNode* Catalog::properties() const
{
    return m_props.ptr();
}

void Catalog::parseTimestamp()
{
    SGPath timestampFile = m_installRoot;
    timestampFile.append(".timestamp");
    sg_ifstream f(timestampFile, std::ios::in);
    f >> m_retrievedTime;
}

void Catalog::writeTimestamp()
{
    SGPath timestampFile = m_installRoot;
    timestampFile.append(".timestamp");
    sg_ofstream f(timestampFile, std::ios::out | std::ios::trunc);
    f << m_retrievedTime << std::endl;
}

unsigned int Catalog::ageInSeconds() const
{
    time_t now;
    time(&now);
    int diff = ::difftime(now, m_retrievedTime);
    return (diff < 0) ? 0 : diff;
}

bool Catalog::needsRefresh() const
{
    // always refresh in these cases
    if ((m_status == Delegate::FAIL_VERSION) || (m_status == Delegate::FAIL_DOWNLOAD)) {
        return true;
    }

    unsigned int maxAge = m_props->getIntValue("max-age-sec", m_root->maxAgeSeconds());
    return (ageInSeconds() > maxAge);
}

std::string Catalog::getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const
{
    if (!aRoot) {
        return std::string();
    }

    if (aRoot->hasChild(m_root->getLocale())) {
        const SGPropertyNode* localeRoot = aRoot->getChild(m_root->getLocale().c_str());
        if (localeRoot->hasChild(aName)) {
            return localeRoot->getStringValue(aName);
        }
    }

    return aRoot->getStringValue(aName);
}

void Catalog::refreshComplete(Delegate::StatusCode aReason)
{
    m_refreshRequest.reset();
    changeStatus(aReason);
}

void Catalog::changeStatus(Delegate::StatusCode newStatus)
{
    if (m_status == newStatus) {
        return;
    }

    m_status = newStatus;
    m_root->catalogRefreshStatus(this, newStatus);
    m_statusCallbacks(this);
}

void Catalog::addStatusCallback(const Callback& cb)
{
    m_statusCallbacks.push_back(cb);
}

Delegate::StatusCode Catalog::status() const
{
    return m_status;
}

} // of namespace pkg

} // of namespace simgear
