## Steve's Super Fun SLAM Time (ie Slam Toolbox)

Fork on slam_karto. I've renamed since really there's about 5% of the code that's the same, everything else has been restructured or removed entirely. 

### Solver Plugins

I have generated pluginlib plugins for a few different solvers that people might be interested in. I like to swap them out for benchmarking and make sure its the same code running for all. I have supported G2O, SPA, and GTSAM. I'm probably going to wrap my own Ceres one as well since no one's open-sourced it but I know _certain_ people are using it. 

GTSAM is currently "unsupported" although all the code is there. I was having infinite problems linking against the library. If someone wants to PR a fix, I can add support but I'm done trying.

All the plugins obviously have dependencies. GTSAM has that + MKL, SPA has ROS dependency of SBA, Ceres has ceres, and G2O has G2O. All aren't hard to install but if you only care about one, you might want to kill the compilation of the others so you can ignore installing them. 

I have spent a extremely long time working with Ceres to optimize it for creation of massive maps, so unless you feel your application is very unique, I'd use my recommended settings. 

### Tools

Manual loop closure, pausing and resuming SLAM, interspection and modification of the pose graph, synchronous SLAM (no missing laser scans) for online or offline application alike without change, more exposed options, rviz plugin for visual interspection and assisting in mapping

### Performance

It can map _very_ large spaces with reasonable CPU and memory consumption. Default settings increase O(N) on number of elements in the pose graph. I recommend from extensive testing to use the SPARSE_NORMAL_CHOLESKY solver with Ceres and the SCHUR_JACOBI preconditioner. Using LM at the trust region strategy is comparable to the dogleg subspace strategy, but LM is much better supported so why argue with it. 

You can get away without a loss function if your odometry is good (ie likelihood for outliers is extremely low). If you have an abnormal application, I might recommend a HuberLoss function.

### Metrics

If you're a weirdo like me and you want to see how I came up with the settings I had, see below.

![ceres_solver_comparison](https://user-images.githubusercontent.com/14944147/41320076-295d3dae-6e53-11e8-82c3-39da7667f45c.png)

I won't go over much of what each data set represents, as that is not open source, but suffice to say that the settings I recommend work well on a large dataset I think anyone would be hardset in a normal application to exceed or find that another solver type is better.