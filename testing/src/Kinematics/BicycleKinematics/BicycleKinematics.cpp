#include "BicycleKinematics.h"

BicycleKinematics::BicycleKinematics()
{
    // Unit vectors
    e1 << 1, 0, 0;
    e2 << 0, 1, 0;
    e3 << 0, 0, 1;
    l2 = 0;                  // Distance from the needle tip frame to the middle of the bicycle
    max_curvature = kConstants.max_curvature; // Natural curvature of the needle. This value is determined experimentally
    v1 = Eigen::VectorXd::Zero(6);
    v1 << e3, max_curvature * e1; // Insertion velocity vector
    v2 = Eigen::VectorXd::Zero(6);
    v2 << 0, 0, 0, e3; // Rotation velocity vector
}
Eigen::Matrix<double, 4, 4, Eigen::DontAlign> BicycleKinematics::ForwardKinematicsBicycleModel(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &transformation_mtx,
                                                                 const double &du1, const double &du2)
{
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> converted_mtx = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
    converted_mtx = ConvertToSpecialEuclideanMatrix((du1 * v1) + (du2 * v2));
    return transformation_mtx * CalcSpecialEuclideanMatrix(converted_mtx);
}

/*!
    This function applies a transformation about desired axis and returns the
    new rotated 4x4 transformation.
    Input: 3x1 vector corresponding to desired rotation amount about x,y,z axis in degrees
*/
Eigen::Matrix<double, 4, 4, Eigen::DontAlign> BicycleKinematics::ApplyRotationEulerAngles(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &trans, const Eigen::Matrix<double, 3, 1, Eigen::DontAlign> &theta)
{
    // Calculating Rotation about x,y,z axis
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> rotation_x;
    rotation_x << 1., 0., 0., 0.,
        0., cos(theta(0)), -sin(theta(0)), 0.,
        0., sin(theta(0)), cos(theta(0)), 0.,
        0., 0., 0., 1;
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> rotation_y;
    rotation_y << cos(theta(1)), 0., sin(theta(1)), 0.,
        0., 1., 0., 0.,
        -sin(theta(1)), 0., cos(theta(1)), 0.,
        0., 0., 0., 1;
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> rotation_z;
    rotation_z << cos(theta(2)), -sin(theta(2)), 0., 0.,
        sin(theta(2)), cos(theta(2)), 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1;
    return (trans * rotation_z * rotation_y * rotation_x);
}

Eigen::Matrix<double, 4, 4, Eigen::DontAlign> BicycleKinematics::ApplyRotationFixedAngles(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &trans, const Eigen::Matrix<double, 3, 1, Eigen::DontAlign> &theta)
{
    // Calculating Rotation about x,y,z axis
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> rotation_x;
    rotation_x << 1., 0., 0.,
        0., cos(theta(0)), -sin(theta(0)),
        0., sin(theta(0)), cos(theta(0));

    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> rotation_y;
    rotation_y << cos(theta(1)), 0., sin(theta(1)),
        0., 1., 0.,
        -sin(theta(1)), 0., cos(theta(1));

    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> rotation_z;
    rotation_z << cos(theta(2)), -sin(theta(2)), 0.,
        sin(theta(2)), cos(theta(2)), 0.,
        0., 0., 1.;
    
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> result = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
    result.block(0,0,3,3) = trans.block(0,0,3,3) * rotation_x * rotation_y * rotation_z;
    return result;
}

/*!
    Computes the exponential map M of a 4x4 se(3) Lie algebra w^.
*/
Eigen::Matrix<double, 4, 4, Eigen::DontAlign> BicycleKinematics::CalcSpecialEuclideanMatrix(const Eigen::Matrix<double, 4, 4, Eigen::DontAlign> &e)
{
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> M = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Identity();
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> w;
    w = e.block(0, 0, 3, 3);
    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> t;
    t << e(0, 3), e(1, 3), e(2, 3);
    // Converting e to exponential map SO(3) form
    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> W = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>::Zero();
    W = ConvertToSpecialOrthagonalVector(w);
    double L = sqrt(pow(W(0), 2) + pow(W(1), 2) + pow(W(2), 2));
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> V = Eigen::Matrix<double, 3, 3, Eigen::DontAlign>::Zero();
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> identity_matrix = Eigen::Matrix<double, 3, 3, Eigen::DontAlign>::Identity();
    V = identity_matrix + (((1 - cos(L)) / pow(L, 2)) * w) + (((L - sin(L)) / pow(L, 3)) * (w * w));
    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> vt = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>::Zero();
    vt = V * t;
    M(0, 3) = vt(0);
    M(1, 3) = vt(1);
    M(2, 3) = vt(2);
    M.block(0, 0, 3, 3) = CalcSpecialOrthagonalMatrix(w);
    return M;
}

/*!
    Computes the exponential map M of 3x3 SO(3) Lie algebra w^.
    w is the skew-symmetric matrix generated by the 3-vector W.
    Angle theta is |W| where W is the 3-vector so(3).
*/
Eigen::Matrix<double, 3, 3, Eigen::DontAlign> BicycleKinematics::CalcSpecialOrthagonalMatrix(const Eigen::Matrix<double, 3, 3, Eigen::DontAlign> &w)
{
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> identity_mtx = Eigen::Matrix<double, 3, 3, Eigen::DontAlign>::Identity();
    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> W = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>::Zero();
    W = ConvertToSpecialOrthagonalVector(w);
    double L = sqrt(pow(W(0), 2) + pow(W(1), 2) + pow(W(2), 2));
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> M = Eigen::Matrix<double, 3, 3, Eigen::DontAlign>::Zero();
    M = identity_mtx + (sin(L) / L * w) + (((1 - cos(L)) / pow(L, 2)) * (w * w));
    return M;
}

/*!
    Converts 3x3 so(3) Lie algebra w to 3x1 SO(3) W
*/
Eigen::Matrix<double, 3, 1, Eigen::DontAlign> BicycleKinematics::ConvertToSpecialOrthagonalVector(const Eigen::Matrix<double, 3, 3, Eigen::DontAlign> &w)
{

    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> W = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>::Zero();
    W << w(2, 1), w(0, 2), w(1, 0);
    return W;
}

/*!
    Converts six element SE(3) matrix W to 4x4 se(3) Lie algebra w
*/
Eigen::Matrix<double, 4, 4, Eigen::DontAlign> BicycleKinematics::ConvertToSpecialEuclideanMatrix(const Eigen::VectorXd &W)
{
    Eigen::Matrix<double, 4, 4, Eigen::DontAlign> w = Eigen::Matrix<double, 4, 4, Eigen::DontAlign>::Zero();
    Eigen::Matrix<double, 3, 1, Eigen::DontAlign> v = Eigen::Matrix<double, 3, 1, Eigen::DontAlign>::Zero();
    v(0) = W(3);
    v(1) = W(4);
    v(2) = W(5);
    w.block(0, 0, 3, 3) = ConvertToSpecialOrthagonalMatrix(v);
    w(0, 3) = W(0);
    w(1, 3) = W(1);
    w(2, 3) = W(2);
    return w;
}

/*!
    This function converts three element SO(3) vector W to 3x3 so(3) Lie algebra w
*/
Eigen::Matrix<double, 3, 3, Eigen::DontAlign> BicycleKinematics::ConvertToSpecialOrthagonalMatrix(const Eigen::Matrix<double, 3, 1, Eigen::DontAlign> &W)
{
    Eigen::Matrix<double, 3, 3, Eigen::DontAlign> w;
    w << 0., -W(2), W(1),
        W(2), 0., -W(0),
        -W(1), W(0), 0.;

    return w;
}