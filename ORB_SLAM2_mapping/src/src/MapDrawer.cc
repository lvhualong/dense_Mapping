/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MapDrawer.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include <pangolin/pangolin.h>
#include <mutex>

namespace ORB_SLAM2
{

static FColorMap colorMap = FColorMap(64) ;

cv::Vec3b computePubPointAndColor(float d)
{
    d += 4 ;
    if ( d < 0 ){
        d = 0 ;
    }
    if ( d > 6 ){
        d = 6 ;
    }
    int i0 = d/6*62 ;
    return colorMap.at(i0) ;
}


MapDrawer::MapDrawer(Map* pMap, const string &strSettingPath):mpMap(pMap)
{
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

    mKeyFrameSize = fSettings["Viewer.KeyFrameSize"];
    mKeyFrameLineWidth = fSettings["Viewer.KeyFrameLineWidth"];
    mGraphLineWidth = fSettings["Viewer.GraphLineWidth"];
    mPointSize = fSettings["Viewer.PointSize"];
    mCameraSize = fSettings["Viewer.CameraSize"];
    mCameraLineWidth = fSettings["Viewer.CameraLineWidth"];
    mMeshLineWidth = fSettings["Viewer.MeshLineWidth"] ;
}

void MapDrawer::DrawMapPoints()
{
    const vector<MapPoint*> &vpMPs = mpMap->GetAllMapPoints();
    const vector<MapPoint*> &vpRefMPs = mpMap->GetReferenceMapPoints();

    set<MapPoint*> spRefMPs(vpRefMPs.begin(), vpRefMPs.end());

    if(vpMPs.empty())
        return;

    glPointSize(mPointSize);
    glBegin(GL_POINTS);
    glColor3f(0.0,0.0,0.0);

    for(size_t i=0, iend=vpMPs.size(); i<iend;i++)
    {
        if(vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))
            continue;
        cv::Mat pos = vpMPs[i]->GetWorldPos();
        glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));
    }
    glEnd();

    glPointSize(mPointSize);
    glBegin(GL_POINTS);
    glColor3f(1.0,0.0,0.0);

    for(set<MapPoint*>::iterator sit=spRefMPs.begin(), send=spRefMPs.end(); sit!=send; sit++)
    {
        if((*sit)->isBad())
            continue;
        cv::Mat pos = (*sit)->GetWorldPos();
        glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));

    }

    glEnd();
}

void MapDrawer::DrawVertexes( bool flag )
{
    if ( flag == false ){
        return ;
    }
    glPointSize(mPointSize);
    glColor3f(221.0/255,30.0/255,244.0/255);
    glBegin(GL_POINTS);

    vector<myPoint3f> points ;
    {
        unique_lock<mutex> lock(mpLocalMesher->mMutexMesh);
        points = mpLocalMesher->points ;
    }
    for(size_t i=0, sz = points.size(); i<sz; i++){
        glVertex3f(points[i].x, points[i].y, points[i].z);
    }

    glEnd();
}

void MapDrawer::DrawTetrahedra( bool flag, double y_base )
{
    if ( flag == false ){
        return ;
    }
    glLineWidth(mMeshLineWidth);
    glShadeModel(GL_SMOOTH);
    //glColor4f(0.8f,0.8f,0.0f, 0.1f);
    glBegin(GL_TRIANGLES);

    vector<myTriangles> tris ;
    {
        unique_lock<mutex> lock(mpLocalMesher->mMutexMesh);
        tris = mpLocalMesher->tris ;
    }
    for(size_t i=0, sz = tris.size(); i<sz; i++)
    {
        cv::Vec3b color ;
        color = computePubPointAndColor(tris[i].p0.y-y_base) ;
        glColor4f(color[0]/255.0, color[1]/255.0, color[2]/255.0, 1.0f);
        glVertex3f(tris[i].p0.x, tris[i].p0.y, tris[i].p0.z);

        color = computePubPointAndColor(tris[i].p1.y-y_base) ;
        glColor4f(color[0]/255.0, color[1]/255.0, color[2]/255.0, 1.0f);
        glVertex3f(tris[i].p1.x, tris[i].p1.y, tris[i].p1.z);

        color = computePubPointAndColor(tris[i].p2.y-y_base) ;
        glColor4f(color[0]/255.0, color[1]/255.0, color[2]/255.0, 1.0f);
        glVertex3f(tris[i].p2.x, tris[i].p2.y, tris[i].p2.z);
    }

    glEnd();
}

void MapDrawer::DrawSpace(bool flag)
{
    if ( flag == false ){
        return ;
    }
    glLineWidth(1);
    glColor3f(1.0f, 80.0/255, 0.0f);
    glBegin(GL_LINES);

    vector<pair<myPoint3f,myPoint3f>> edges ;
    {
        unique_lock<mutex> lock(mpLocalMesher->mMutexMesh);
        edges = mpLocalMesher->edges ;
    }
    for(size_t i=0, sz = edges.size(); i<sz; i++)
    {
        glVertex3f(edges[i].first.x, edges[i].first.y, edges[i].first.z);
        glVertex3f(edges[i].second.x, edges[i].second.y, edges[i].second.z);

//        ROS_WARN("%f %f %f %f %f %f", edges[i].first.x, edges[i].first.y ,edges[i].first.z,
//                 edges[i].second.x, edges[i].second.y, edges[i].second.z ) ;
    }

    glEnd();
}

void MapDrawer::DrawMesh( bool flag )
{
    if ( flag == false ){
        return ;
    }

    glLineWidth(mMeshLineWidth);
    glColor3f(1.0f, 80.0/255,0.0f);
    glBegin(GL_LINES);

    vector<myTriangles> tris ;
    {
        unique_lock<mutex> lock(mpLocalMesher->mMutexMesh);
        tris = mpLocalMesher->tris ;
    }
    for(size_t i=0, sz = tris.size(); i<sz; i++)
    {
        glVertex3f(tris[i].p0.x, tris[i].p0.y, tris[i].p0.z);
        glVertex3f(tris[i].p1.x, tris[i].p1.y, tris[i].p1.z);
        glVertex3f(tris[i].p1.x, tris[i].p1.y, tris[i].p1.z);
        glVertex3f(tris[i].p2.x, tris[i].p2.y, tris[i].p2.z);
        glVertex3f(tris[i].p0.x, tris[i].p0.y, tris[i].p0.z);
        glVertex3f(tris[i].p2.x, tris[i].p2.y, tris[i].p2.z);
    }

    glEnd();
}

void MapDrawer::DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph)
{
    const float &w = mKeyFrameSize;
    const float h = w*0.75;
    const float z = w*0.6;

    const vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();

    if(bDrawKF)
    {
        for(size_t i=0; i<vpKFs.size(); i++)
        {
            KeyFrame* pKF = vpKFs[i];
            cv::Mat Twc = pKF->GetPoseInverse().t();

            glPushMatrix();

            glMultMatrixf(Twc.ptr<GLfloat>(0));

            glLineWidth(mKeyFrameLineWidth);
            glColor3f(0.0f,0.0f,1.0f);
            glBegin(GL_LINES);
            glVertex3f(0,0,0);
            glVertex3f(w,h,z);
            glVertex3f(0,0,0);
            glVertex3f(w,-h,z);
            glVertex3f(0,0,0);
            glVertex3f(-w,-h,z);
            glVertex3f(0,0,0);
            glVertex3f(-w,h,z);

            glVertex3f(w,h,z);
            glVertex3f(w,-h,z);

            glVertex3f(-w,h,z);
            glVertex3f(-w,-h,z);

            glVertex3f(-w,h,z);
            glVertex3f(w,h,z);

            glVertex3f(-w,-h,z);
            glVertex3f(w,-h,z);
            glEnd();

            glPopMatrix();
        }
    }

    if(bDrawGraph)
    {
        glLineWidth(mGraphLineWidth);
        glColor4f(0.0f,1.0f,0.0f,0.6f);
        glBegin(GL_LINES);

        for(size_t i=0; i<vpKFs.size(); i++)
        {
            // Covisibility Graph
            const vector<KeyFrame*> vCovKFs = vpKFs[i]->GetCovisiblesByWeight(100);
            cv::Mat Ow = vpKFs[i]->GetCameraCenter();
            if(!vCovKFs.empty())
            {
                for(vector<KeyFrame*>::const_iterator vit=vCovKFs.begin(), vend=vCovKFs.end(); vit!=vend; vit++)
                {
                    if((*vit)->mnId<vpKFs[i]->mnId)
                        continue;
                    cv::Mat Ow2 = (*vit)->GetCameraCenter();
                    glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                    glVertex3f(Ow2.at<float>(0),Ow2.at<float>(1),Ow2.at<float>(2));
                }
            }

            // Spanning tree
            KeyFrame* pParent = vpKFs[i]->GetParent();
            if(pParent)
            {
                cv::Mat Owp = pParent->GetCameraCenter();
                glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                glVertex3f(Owp.at<float>(0),Owp.at<float>(1),Owp.at<float>(2));
            }

            // Loops
            set<KeyFrame*> sLoopKFs = vpKFs[i]->GetLoopEdges();
            for(set<KeyFrame*>::iterator sit=sLoopKFs.begin(), send=sLoopKFs.end(); sit!=send; sit++)
            {
                if((*sit)->mnId<vpKFs[i]->mnId)
                    continue;
                cv::Mat Owl = (*sit)->GetCameraCenter();
                glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                glVertex3f(Owl.at<float>(0),Owl.at<float>(1),Owl.at<float>(2));
            }
        }

        glEnd();
    }
}

void MapDrawer::DrawCurrentCamera(pangolin::OpenGlMatrix &Twc)
{
    const float &w = mCameraSize;
    const float h = w*0.75;
    const float z = w*0.6;

    glPushMatrix();

#ifdef HAVE_GLES
    glMultMatrixf(Twc.m);
#else
    glMultMatrixd(Twc.m);
#endif

    glLineWidth(mCameraLineWidth);
    glColor3f(0.0f,1.0f,0.0f);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(w,h,z);
    glVertex3f(0,0,0);
    glVertex3f(w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,h,z);

    glVertex3f(w,h,z);
    glVertex3f(w,-h,z);

    glVertex3f(-w,h,z);
    glVertex3f(-w,-h,z);

    glVertex3f(-w,h,z);
    glVertex3f(w,h,z);

    glVertex3f(-w,-h,z);
    glVertex3f(w,-h,z);
    glEnd();

    glPopMatrix();
}


void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw)
{
    unique_lock<mutex> lock(mMutexCamera);
    mCameraPose = Tcw.clone();
}

void MapDrawer::GetCurrentOpenGLCameraMatrix(pangolin::OpenGlMatrix &M)
{
    if(!mCameraPose.empty())
    {
        cv::Mat Rwc(3,3,CV_32F);
        cv::Mat twc(3,1,CV_32F);
        {
            unique_lock<mutex> lock(mMutexCamera);
            Rwc = mCameraPose.rowRange(0,3).colRange(0,3).t();
            twc = -Rwc*mCameraPose.rowRange(0,3).col(3);
        }

        M.m[0] = Rwc.at<float>(0,0);
        M.m[1] = Rwc.at<float>(1,0);
        M.m[2] = Rwc.at<float>(2,0);
        M.m[3]  = 0.0;

        M.m[4] = Rwc.at<float>(0,1);
        M.m[5] = Rwc.at<float>(1,1);
        M.m[6] = Rwc.at<float>(2,1);
        M.m[7]  = 0.0;

        M.m[8] = Rwc.at<float>(0,2);
        M.m[9] = Rwc.at<float>(1,2);
        M.m[10] = Rwc.at<float>(2,2);
        M.m[11]  = 0.0;

        M.m[12] = twc.at<float>(0);
        M.m[13] = twc.at<float>(1);
        M.m[14] = twc.at<float>(2);
        M.m[15]  = 1.0;
    }
    else
        M.SetIdentity();
}

} //namespace ORB_SLAM
