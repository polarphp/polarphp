<?php
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/28.

namespace Gyb\Kernel;

use Symfony\Bundle\FrameworkBundle\Kernel\MicroKernelTrait;
use Symfony\Component\Config\Loader\LoaderInterface;
use Symfony\Component\Config\Resource\FileResource;
use Symfony\Component\DependencyInjection\ContainerBuilder;
use Symfony\Component\HttpKernel\Kernel as BaseKernel;
use Symfony\Component\Routing\RouteCollectionBuilder;

class Kernel extends BaseKernel
{
   use MicroKernelTrait;

   private const CONFIG_EXTS = '.{php,xml,yaml,yml}';

   public function registerBundles(): iterable
   {
      $contents = require $this->getProjectDir().'/config/bundles.php';
      foreach ($contents as $class => $envs) {
         if ($envs[$this->environment] ?? $envs['all'] ?? false) {
            yield new $class();
         }
      }
   }

   public function getProjectDir(): string
   {
      return GYB_ROOT_DIR;
   }

   protected function configureContainer(ContainerBuilder $container, LoaderInterface $loader): void
   {
      $container->addResource(new FileResource($this->getProjectDir().'/config/bundles.php'));
      $container->setParameter('container.dumper.inline_class_loader', true);
      $confDir = $this->getProjectDir().'/config';
      $loader->load($confDir.'/{packages}/*'.self::CONFIG_EXTS, 'glob');
      $loader->load($confDir.'/{packages}/'.$this->environment.'/**/*'.self::CONFIG_EXTS, 'glob');
      $loader->load($confDir.'/{services}'.self::CONFIG_EXTS, 'glob');
      $loader->load($confDir.'/{services}_'.$this->environment.self::CONFIG_EXTS, 'glob');
   }

   protected function configureRoutes(RouteCollectionBuilder $routes): void
   {
   }
}
