#include "features_matcher.h"

#include <iostream>
#include <map>

// Read from file
#include <fstream>
#include <string>

FeatureMatcher::FeatureMatcher(cv::Mat intrinsics_matrix, cv::Mat dist_coeffs, double focal_scale)
{
  intrinsics_matrix_ = intrinsics_matrix.clone();
  dist_coeffs_ = dist_coeffs.clone();
  new_intrinsics_matrix_ = intrinsics_matrix.clone();
  new_intrinsics_matrix_.at<double>(0,0) *= focal_scale;
  new_intrinsics_matrix_.at<double>(1,1) *= focal_scale;
}

cv::Mat FeatureMatcher::readUndistortedImage(const std::string& filename )
{
  cv::Mat img = cv::imread(filename), und_img, dbg_img;
  cv::undistort	(	img, und_img, intrinsics_matrix_, dist_coeffs_, new_intrinsics_matrix_ );

  return und_img;
}

void FeatureMatcher::extractFeatures()
{
  features_.resize(images_names_.size());
  descriptors_.resize(images_names_.size());
  feats_colors_.resize(images_names_.size());
  

  for( int i = 0; i < images_names_.size(); i++  )
  {
    std::cout<<"Computing descriptors for image "<<i<<std::endl;
    cv::Mat img = readUndistortedImage(images_names_[i]);

    //////////////////////////// Code to be completed (1/6) /////////////////////////////////
    // Extract salient points + descriptors from i-th image, and store them into
    // features_[i] and descriptors_[i] vector, respectively
    // Extract also the color (i.e., the cv::Vec3b information) of each feature, and store
    // it into feats_colors_[i] vector
    /////////////////////////////////////////////////////////////////////////////////////////
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    cv::Mat mask = cv::Mat::ones(img.rows, img.cols, CV_8UC1);

    sift->detectAndCompute(img, mask, features_[i], descriptors_[i]);

    for(int j = 0; j < features_[i].size(); j++)
        feats_colors_[i].push_back(img.at<cv::Vec3b>((int)features_[i][j].pt.y,(int)features_[i][j].pt.x));
    /////////////////////////////////////////////////////////////////////////////////////////
  }
}

void FeatureMatcher::exhaustiveMatching()
{
  for( int i = 0; i < images_names_.size() - 1; i++ )
  {
    for( int j = i + 1; j < images_names_.size(); j++ )
    {
      std::cout<<"Matching image "<<i<<" with image "<<j<<std::endl;
      std::vector<cv::DMatch> matches, inlier_matches;

      //////////////////////////// Code to be completed (2/6) /////////////////////////////////
      // Match descriptors between image i and image j, and perform geometric validation,
      // possibly discarding the outliers (remember that features have been extracted
      // from undistorted images that now has new_intrinsics_matrix_ as K matrix and
      // no distortions)
      // As geometric models, use both the Essential matrix and the Homograph matrix,
      // both by setting new_intrinsics_matrix_ as K matrix.
      // As threshold in the functions to estimate both models, you may use 1.0 or similar.
      // Store inlier matches into the inlier_matches vector
      // Do not set matches between two images if the amount of inliers matches
      // (i.e., geomatrically verified matches) is small (say <= 5 matches)
      // In case of success, set the matches with the function:
      // setMatches( i, j, inlier_matches);
      /////////////////////////////////////////////////////////////////////////////////////////

      bool super_glue = false;
      std::vector<cv::Point2f> p_i, p_j;

      if(!super_glue) {
          std::vector<cv::DMatch> raw_matches;
          cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::BRUTEFORCE);
          matcher->match(descriptors_[i], descriptors_[j], raw_matches);

          double ratio = 5.0;
          double min_distance = raw_matches[0].distance;

          for (int k = 1; k < raw_matches.size(); k++) {
              if (raw_matches[k].distance < min_distance)
                  min_distance = raw_matches[k].distance;
          }

          for (auto &raw_match: raw_matches) {
              if (raw_match.distance < ratio * min_distance)
                  matches.push_back(raw_match);
          }

      }
      else { // Load SuperGlue matches and keypoints

          // clear previous features and features colors extracted
          features_[i].clear();
          features_[j].clear();
          feats_colors_[i].clear();
          feats_colors_[j].clear();

          std::string dataset = "3"; // set according to dataset considered
          // collect filename of the file to open according to current image couple considered
          std::string path = "../datasets/superglue/match_info_" + dataset + "/";
          std::string first_img = (i < 10) ? ("0" + std::to_string(i)) : std::to_string(i);
          std::string second_img = (j < 10) ? ("0" + std::to_string(j)) : std::to_string(j);
          std::string filename = path + first_img + "_" + second_img + ".txt";

          std::string line;
          bool matches_collected = false;
          // open file stream
          std::ifstream file(filename);

          if (file.is_open()) {
              // while file is okay
              while (file) {

                  std::getline(file, line); // read line

                  if (!line.empty()) { //if the line is non-empty then

                      // tokenize current line using space as delimiter
                      std::string delimiter = " ";
                      size_t pos = 0;
                      std::vector<std::string> tokens;

                      // collect tokens from current line
                      while ((pos = line.find(delimiter)) != std::string::npos) {
                          tokens.push_back(line.substr(0, pos));
                          line.erase(0, pos + delimiter.length());
                      }
                      tokens.push_back(line.substr(0, pos));

                      // load matches first if not already done
                      if (!matches_collected) {

                          int imgIdx = std::stoi(tokens[0]);
                          int trainIdx = std::stoi(tokens[1]);
                          int queryIdx = std::stoi(tokens[2]);
                          float distance = std::stof(tokens[3]);

                          cv::DMatch match = cv::DMatch(queryIdx, trainIdx, imgIdx, distance);
                          matches.push_back(match);
                      }

                      // else, we have to load keypoints
                      else {
                          int keypoint_size = 1;
                          int k = std::stoi(tokens[0]);
                          float x = std::stof(tokens[1]);
                          float y = std::stof(tokens[2]);

                          if (k == 0)
                            features_[i].emplace_back(cv::Point2f(x,y),keypoint_size);
                          else
                            features_[j].emplace_back(cv::Point2f(x,y),keypoint_size);

                      }
                  }
                  else // if the line is empty, we collected all matches, start with keypoints
                      matches_collected = true;
              }
          }

          // need to open images i,j to re-collect color information according to the new keypoints considered
          cv::Mat img_src = cv::imread("../datasets/images_" + dataset + "/" + first_img + ".jpg");
          cv::Mat img_dst = cv::imread("../datasets/images_" + dataset + "/" + second_img + ".jpg");

          for(int z = 0; z < features_[i].size(); z++)
              feats_colors_[i].push_back(img_src.at<cv::Vec3b>((int)features_[i][z].pt.y,(int)features_[i][z].pt.x));
          for(int z = 0; z < features_[j].size(); z++)
              feats_colors_[j].push_back(img_dst.at<cv::Vec3b>((int)features_[j][z].pt.y,(int)features_[j][z].pt.x));


          // visualize SuperGlue matches without geometric verification
          bool viz_sg = false;
          if(viz_sg) {
              cv::Mat result;
              cv::drawMatches(img_src, features_[i], img_dst, features_[j], matches, result);
              cv::imshow("SuperGlue matches", result);
              cv::waitKey(400);
          }
      }

      // prepare points that corresponds to a match
      for (auto &match: matches) {
            p_i.push_back(features_[i][match.queryIdx].pt);
            p_j.push_back(features_[j][match.trainIdx].pt);
      }

      // perform geometric verification on points extracted above
      double threshold = 1.0;
      double prob = 0.999;
      cv::Mat essential_mask, homography_mask;

      cv::Mat E = cv::findEssentialMat(p_i, p_j, new_intrinsics_matrix_, cv::RANSAC, prob, threshold, essential_mask);
      cv::Mat H = cv::findHomography(p_i, p_j, cv::RANSAC, threshold, homography_mask);

      for(int k = 0; k < essential_mask.rows; k++)
          if(essential_mask.at<unsigned char>(k) == 1)
              inlier_matches.push_back(matches[k]);

      for(int k = 0; k < homography_mask.rows; k++)
          if(homography_mask.at<unsigned char>(k) == 1)
              inlier_matches.push_back(matches[k]);

      int min_inliers = 5;
      if(inlier_matches.size() > min_inliers)
          setMatches( i, j, inlier_matches);
      /////////////////////////////////////////////////////////////////////////////////////////
    }
  }
}

void FeatureMatcher::writeToFile ( const std::string& filename, bool normalize_points ) const
{
  FILE* fptr = fopen(filename.c_str(), "w");

  if (fptr == NULL) {
    std::cerr << "Error: unable to open file " << filename;
    return;
  };

  fprintf(fptr, "%d %d %d\n", num_poses_, num_points_, num_observations_);

  double *tmp_observations;
  cv::Mat dst_pts;
  if(normalize_points)
  {
    cv::Mat src_obs( num_observations_,1, cv::traits::Type<cv::Vec2d>::value,
                     const_cast<double *>(observations_.data()));
    cv::undistortPoints(src_obs, dst_pts, new_intrinsics_matrix_, cv::Mat());
    tmp_observations = reinterpret_cast<double *>(dst_pts.data);
  }
  else
  {
    tmp_observations = const_cast<double *>(observations_.data());
  }

  for (int i = 0; i < num_observations_; ++i)
  {
    fprintf(fptr, "%d %d", pose_index_[i], point_index_[i]);
    for (int j = 0; j < 2; ++j) {
      fprintf(fptr, " %g", tmp_observations[2 * i + j]);
    }
    fprintf(fptr, "\n");
  }

  if( colors_.size() == 3*num_points_ )
  {
    for (int i = 0; i < num_points_; ++i)
      fprintf(fptr, "%d %d %d\n", colors_[i*3], colors_[i*3 + 1], colors_[i*3 + 2]);
  }

  fclose(fptr);
}

void FeatureMatcher::testMatches( double scale )
{
  // For each pose, prepare a map that reports the pairs [point index, observation index]
  std::vector< std::map<int,int> > cam_observation( num_poses_ );
  for( int i_obs = 0; i_obs < num_observations_; i_obs++ )
  {
    int i_cam = pose_index_[i_obs], i_pt = point_index_[i_obs];
    cam_observation[i_cam][i_pt] = i_obs;
  }

  for(int r = 0; r < num_poses_; r++)
  {
    for (int c = r + 1; c < num_poses_; c++)
    {
      int num_mathces = 0;
      std::vector<cv::DMatch> matches;
      std::vector<cv::KeyPoint> features0, features1;
      for (auto const &co_iter: cam_observation[r])
      {
        if (cam_observation[c].find(co_iter.first) != cam_observation[c].end())
        {
          features0.emplace_back(observations_[2*co_iter.second],observations_[2*co_iter.second + 1], 0.0);
          features1.emplace_back(observations_[2*cam_observation[c][co_iter.first]],observations_[2*cam_observation[c][co_iter.first] + 1], 0.0);
          matches.emplace_back(num_mathces,num_mathces, 0);
          num_mathces++;
        }
      }
      cv::Mat img0 = readUndistortedImage(images_names_[r]),
          img1 = readUndistortedImage(images_names_[c]),
          dbg_img;

      cv::drawMatches(img0, features0, img1, features1, matches, dbg_img);
      cv::resize(dbg_img, dbg_img, cv::Size(), scale, scale);
      cv::imshow("", dbg_img);
      if (cv::waitKey() == 27)
        return;
    }
  }
}

void FeatureMatcher::setMatches( int pos0_id, int pos1_id, const std::vector<cv::DMatch> &matches )
{

  const auto &features0 = features_[pos0_id];
  const auto &features1 = features_[pos1_id];

  auto pos_iter0 = pose_id_map_.find(pos0_id),
      pos_iter1 = pose_id_map_.find(pos1_id);

  // Already included position?
  if( pos_iter0 == pose_id_map_.end() )
  {
    pose_id_map_[pos0_id] = num_poses_;
    pos0_id = num_poses_++;
  }
  else
    pos0_id = pose_id_map_[pos0_id];

  // Already included position?
  if( pos_iter1 == pose_id_map_.end() )
  {
    pose_id_map_[pos1_id] = num_poses_;
    pos1_id = num_poses_++;
  }
  else
    pos1_id = pose_id_map_[pos1_id];

  for( auto &match:matches)
  {

    // Already included observations?
    uint64_t obs_id0 = poseFeatPairID(pos0_id, match.queryIdx ),
        obs_id1 = poseFeatPairID(pos1_id, match.trainIdx );
    auto pt_iter0 = point_id_map_.find(obs_id0),
        pt_iter1 = point_id_map_.find(obs_id1);
    // New point
    if( pt_iter0 == point_id_map_.end() && pt_iter1 == point_id_map_.end() )
    {
      int pt_idx = num_points_++;
      point_id_map_[obs_id0] = point_id_map_[obs_id1] = pt_idx;

      point_index_.push_back(pt_idx);
      point_index_.push_back(pt_idx);
      pose_index_.push_back(pos0_id);
      pose_index_.push_back(pos1_id);
      observations_.push_back(features0[match.queryIdx].pt.x);
      observations_.push_back(features0[match.queryIdx].pt.y);
      observations_.push_back(features1[match.trainIdx].pt.x);
      observations_.push_back(features1[match.trainIdx].pt.y);

      // Average color between two corresponding features (suboptimal since we shouls also consider
      // the other observations of the same point in the other images)
      cv::Vec3f color = (cv::Vec3f(feats_colors_[pos0_id][match.queryIdx]) +
                        cv::Vec3f(feats_colors_[pos1_id][match.trainIdx]))/2;

      colors_.push_back(cvRound(color[2]));
      colors_.push_back(cvRound(color[1]));
      colors_.push_back(cvRound(color[0]));

      num_observations_++;
      num_observations_++;
    }
      // New observation
    else if( pt_iter0 == point_id_map_.end() )
    {
      int pt_idx = point_id_map_[obs_id1];
      point_id_map_[obs_id0] = pt_idx;

      point_index_.push_back(pt_idx);
      pose_index_.push_back(pos0_id);
      observations_.push_back(features0[match.queryIdx].pt.x);
      observations_.push_back(features0[match.queryIdx].pt.y);
      num_observations_++;
    }
    else if( pt_iter1 == point_id_map_.end() )
    {
      int pt_idx = point_id_map_[obs_id0];
      point_id_map_[obs_id1] = pt_idx;

      point_index_.push_back(pt_idx);
      pose_index_.push_back(pos1_id);
      observations_.push_back(features1[match.trainIdx].pt.x);
      observations_.push_back(features1[match.trainIdx].pt.y);
      num_observations_++;
    }
//    else if( pt_iter0->second != pt_iter1->second )
//    {
//      std::cerr<<"Shared observations does not share 3D point!"<<std::endl;
//    }
  }
}
void FeatureMatcher::reset()
{
  point_index_.clear();
  pose_index_.clear();
  observations_.clear();
  colors_.clear();

  num_poses_ = num_points_ = num_observations_ = 0;
}
