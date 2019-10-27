<?php
/**
 * @author https://github.com/andersondanilo
 */
namespace Lit\ProcessPool;

use Iterator;
use Lit\ProcessPool\Events\ProcessEvent;
use Lit\ProcessPool\Events\ProcessFinished;
use Lit\ProcessPool\Events\ProcessStarted;
use Symfony\Component\Process\Exception\RuntimeException;
use Symfony\Component\Process\Process;
use Symfony\Component\EventDispatcher\EventDispatcher;
use function Lit\Utils\detect_cpus;

/**
 * Process pool allow you to run a constant number
 * of parallel processes
 */
class ProcessPool
{
   /**
    * @var array $queue
    */
   private $queue;
   /**
    * Running processes
    *
    * @var array $running
    */
   private $running = [];
   /**
    * @var array $options
    */
   private $options;
   /**
    * @var int $concurrency
    */
   private $concurrency;
   /**
    * @var EventDispatcher
    */
   private $eventDispatcher;

   /**
    * @var WorkerInitializerInterface $initializer
    */
   private $initializer;

   /**
    * @var bool $closed
    */
   private $closed = false;

   /**
    * @var bool $terminateFlag
    */
   private $terminateFlag = false;

   /**
    * @var float $maxTimeout
    */
   private $maxTimeout = null;

   /**
    * Accept any type of iterator, inclusive Generator
    *
    * @param Process[] $queue
    * @param array $options
    */
   public function __construct(WorkerInitializerInterface $initializer, array $options = [])
   {
      $this->eventDispatcher = new EventDispatcher;
      $this->options = array_merge($this->getDefaultOptions(), $options);
      if (!isset($this->options['concurrency'])) {
         $this->concurrency = detect_cpus();
      } else {
         $this->concurrency = $this->options['concurrency'];
      }
      if (isset($this->options['maxTimeout'])) {
         $maxTimeout = (int)$this->options['maxTimeout'];
         if ($maxTimeout > 0){
            $this->maxTimeout = (float)$maxTimeout * 1000000;
         }
      }
      $this->initializer = $initializer;
   }

   private function getDefaultOptions()
   {
      return [
         'concurrency' => '5',
         'eventPreffix' => 'process_pool',
         'throwExceptions' => false,
      ];
   }

   public function postTask(TaskInterface $task): Process
   {
      if ($this->closed) {
         return null;
      }
      $cmd = [PHP_BINARY, LIT_SCRIPT, 'run-worker'];
      $process = new Process($cmd, null, null);
      $data = array(
         'task' => $task,
         'initializer' => $this->initializer
      );
      $process->setInput(serialize($data));
      $this->queue[] = $process;
      return $process;
   }

   /**
    * Start and wait until all processes finishes
    *
    * @return void
    */
   public function wait()
   {
      if (!$this->terminateFlag) {
         $this->startNextProcesses();
      }
      while (count($this->running) > 0 && !$this->terminateFlag) {
         /** @var Process $process */
         foreach ($this->running as $key => $process) {
            if ($this->maxTimeout !== null) {
               $process->setTimeout(max($this->maxTimeout - microtime(true)));
            }
            $exception = null;
            try {
               $process->checkTimeout();
               $isRunning = $process->isRunning();
            } catch (RuntimeException $e) {
               $isRunning = false;
               $exception = $e;
               if ($this->shouldThrowExceptions()) {
                  throw $e;
               }
            }
            if (!$isRunning) {
               unset($this->running[$key]);
               $this->startNextProcesses();
               $event = new ProcessFinished($process);
               if ($exception) {
                  $event->setException($exception);
               }
               $this->dispatchEvent($event);
            }
         }
         usleep(1000);
      }
      if ($this->terminateFlag && !empty($this->running)) {
         foreach ($this->running as $key => $process) {
            $process->stop(0);
            unset($this->running[$key]);
         }
      }
   }

   public function setProcessFinishedCallback(callable $callback)
   {
      $eventName = $this->options['eventPreffix'] . '.' . ProcessFinished::NAME;
      $this->getEventDispatcher()->addListener($eventName, $callback);
   }

   /**
    * called in process callback
    */
   public function terminate()
   {
      $this->terminateFlag = true;
   }

   /**
    * Start next processes until fill the concurrency limit
    *
    * @return void
    */
   private function startNextProcesses()
   {
      $concurrency = $this->getConcurrency();
      while (count($this->running) < $concurrency && !empty($this->queue)) {
         $process = array_shift($this->queue);
         $process->start();
         $this->dispatchEvent(new ProcessStarted($process));
         $this->running[] = $process;
      }
   }

   private function shouldThrowExceptions()
   {
      return $this->options['throwExceptions'];
   }

   /**
    * Get processes concurrency, default 5
    *
    * @return int
    */
   public function getConcurrency()
   {
      return $this->concurrency;
   }

   public function close(): ProcessPool
   {
      $this->closed = true;
      return $this;
   }

   /**
    * @return bool
    */
   public function isClosed()
   {
      return $this->closed;
   }

   /**
    * @param int $concurrency
    *
    * @return static
    */
   public function setConcurrency($concurrency)
   {
      $this->concurrency = $concurrency;
      return $this;
   }

   private function dispatchEvent(ProcessEvent $event)
   {
      $eventPreffix = $this->options['eventPreffix'];
      $eventName = $event::NAME;
      $this->getEventDispatcher()->dispatch($event, "$eventPreffix.$eventName");
   }

   /**
    * @return EventDispatcher
    */
   public function getEventDispatcher()
   {
      return $this->eventDispatcher;
   }

   /**
    * @return WorkerInitializerInterface
    */
   public function getInitializer(): WorkerInitializerInterface
   {
      return $this->initializer;
   }

   /**
    * @param EventDispatcher $eventDispatcher
    *
    * @return static
    */
   public function setEventDispatcher(EventDispatcher $eventDispatcher)
   {
      $this->eventDispatcher = $eventDispatcher;
      return $this;
   }
}